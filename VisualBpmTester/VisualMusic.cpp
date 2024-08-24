#include "VisualMusic.h"
#include <SFML/System/Err.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace {

constexpr std::size_t buffersize = 441 * 10;
constexpr std::int32_t samplerate = 44100;// 22046; 44092; 88185; 176370;

}

VisualMusic::VisualMusic() :
	m_file(),
	m_loopSpan(0, 0),
	m_pointsPerShape(0),
	m_rectangle(sf::Vector2f(1.0f, 1.0f)) {}

VisualMusic::~VisualMusic() {
	// We must stop before destroying the file
	stop();
}

bool VisualMusic::openFromFile(const std::string& filename) {
	// First stop the music if it was already running
	stop();

	// Open the underlying sound file
	if (!m_file.openFromFile(filename))
		return false;

	// Perform common initializations
	initialize();

	return true;
}

bool VisualMusic::openFromMemory(const void* data, std::size_t sizeInBytes) {
	// First stop the music if it was already running
	stop();

	// Open the underlying sound file
	if (!m_file.openFromMemory(data, sizeInBytes))
		return false;

	// Perform common initializations
	initialize();

	return true;
}

bool VisualMusic::openFromStream(sf::InputStream& stream) {
	// First stop the music if it was already running
	stop();

	// Open the underlying sound file
	if (!m_file.openFromStream(stream))
		return false;

	// Perform common initializations
	initialize();

	return true;
}

sf::Time VisualMusic::getDuration() const {
	return m_file.getDuration();
}

VisualMusic::TimeSpan VisualMusic::getLoopPoints() const {
	return TimeSpan(samplesToTime(m_loopSpan.offset), samplesToTime(m_loopSpan.length));
}

void VisualMusic::setLoopPoints(TimeSpan timePoints) {
	Span<sf::Uint64> samplePoints(timeToSamples(timePoints.offset), timeToSamples(timePoints.length));

	// Check our state. This averts a divide-by-zero. GetChannelCount() is cheap enough to use often
	if (getChannelCount() == 0 || m_file.getSampleCount() == 0) {
		sf::err() << "Music is not in a valid state to assign Loop Points." << std::endl;
		return;
	}

	// Round up to the next even sample if needed
	samplePoints.offset += (getChannelCount() - 1ull);
	samplePoints.offset -= (samplePoints.offset % getChannelCount());
	samplePoints.length += (getChannelCount() - 1ull);
	samplePoints.length -= (samplePoints.length % getChannelCount());

	// Validate
	if (samplePoints.offset >= m_file.getSampleCount()) {
		sf::err() << "LoopPoints offset val must be in range [0, Duration)." << std::endl;
		return;
	}
	if (samplePoints.length == 0) {
		sf::err() << "LoopPoints length val must be nonzero." << std::endl;
		return;
	}

	// Clamp End Point
	samplePoints.length = std::min(samplePoints.length, m_file.getSampleCount() - samplePoints.offset);

	// If this change has no effect, we can return without touching anything
	if (samplePoints.offset == m_loopSpan.offset && samplePoints.length == m_loopSpan.length)
		return;

	// When we apply this change, we need to "reset" this instance and its buffer

	// Get old playing status and position
	Status oldStatus = getStatus();
	sf::Time oldPos = getPlayingOffset();

	// Unload
	stop();

	// Set
	m_loopSpan = samplePoints;

	// Restore
	if (oldPos != sf::Time::Zero)
		setPlayingOffset(oldPos);

	// Resume
	if (oldStatus == Playing)
		play();
}

bool VisualMusic::onGetData(Chunk& data) {
	std::lock_guard lock(m_mutex);

	std::size_t toFill = m_samples.size();
	sf::Uint64 currentOffset = m_file.getSampleOffset();
	sf::Uint64 loopEnd = m_loopSpan.offset + m_loopSpan.length;

	// If the loop end is enabled and imminent, request less data.
	// This will trip an "onLoop()" call from the underlying SoundStream,
	// and we can then take action.
	if (getLoop() && (m_loopSpan.length != 0) && (currentOffset <= loopEnd) && (currentOffset + toFill > loopEnd))
		toFill = static_cast<std::size_t>(loopEnd - currentOffset);

	// Fill the chunk parameters
	data.samples = &m_samples[0];
	data.sampleCount = static_cast<std::size_t>(m_file.read(&m_samples[0], toFill));
	currentOffset += data.sampleCount;

	// Check if we have stopped obtaining samples or reached either the EOF or the loop end point
	return (data.sampleCount != 0) && (currentOffset < m_file.getSampleCount()) && !(currentOffset == loopEnd && m_loopSpan.length != 0);
}

void VisualMusic::onSeek(sf::Time timeOffset) {
	std::lock_guard lock(m_mutex);
	m_file.seek(timeOffset);
}

sf::Int64 VisualMusic::onLoop() {
	// Called by underlying SoundStream so we can determine where to loop.
	std::lock_guard lock(m_mutex);
	sf::Uint64 currentOffset = m_file.getSampleOffset();
	if (getLoop() && (m_loopSpan.length != 0) && (currentOffset == m_loopSpan.offset + m_loopSpan.length)) {
		// Looping is enabled, and either we're at the loop end, or we're at the EOF
		// when it's equivalent to the loop end (loop end takes priority). Send us to loop begin
		m_file.seek(m_loopSpan.offset);
		return static_cast<sf::Int64>(m_file.getSampleOffset());
	}
	else if (getLoop() && (currentOffset >= m_file.getSampleCount())) {
		// If we're at the EOF, reset to 0
		m_file.seek(0);
		return 0;
	}
	return NoLoop;
}

void VisualMusic::initialize() {
	// Compute the music positions
	m_loopSpan.offset = 0;
	m_loopSpan.length = m_file.getSampleCount();

	// Resize the internal buffer so that it can contain 1 second of audio samples
	m_samples.resize((sf::Uint64)m_file.getSampleRate() * m_file.getChannelCount());

	// Initialize the stream
	SoundStream::initialize(m_file.getChannelCount(), m_file.getSampleRate());

	return computeSamples();
}

sf::Uint64 VisualMusic::timeToSamples(sf::Time position) const {
	// Always ROUND, no unchecked truncation, hence the addition in the numerator.
	// This avoids most precision errors arising from "samples => Time => samples" conversions
	// Original rounding calculation is ((Micros * Freq * Channels) / 1000000) + 0.5
	// We refactor it to keep Int64 as the data type throughout the whole operation.
	unsigned int chcnt = getChannelCount();
	if (chcnt == 0) return 0;
	sf::Uint64 res = ((static_cast<sf::Uint64>(position.asMicroseconds()) * getSampleRate() * getChannelCount()) + 500000) / 1000000;
	res -= res % chcnt;
	return res;
}

sf::Time VisualMusic::samplesToTime(uint64_t samples) const {
	sf::Time position = sf::Time::Zero;

	// Make sure we don't divide by 0
	if (getSampleRate() != 0 && getChannelCount() != 0)
		position = sf::microseconds(static_cast<sf::Int64>((samples * 1000000) / ((sf::Uint64)getChannelCount() * getSampleRate())));

	return position;
}


void VisualMusic::setShowRange(bool followMusic, TimeSpan range) {}

void VisualMusic::draw(sf::RenderTarget& target, sf::RenderStates states) const {
	uint32_t sampleRate = getSampleRate();
	uint32_t samplesPerShape = m_pointsPerShape * getChannelCount();

	uint32_t delta = sampleRate * 8;
	delta = delta / samplesPerShape + ((delta % samplesPerShape) ? 1 : 0);

	sf::Time centerTime = getPlayingOffset();
	uint64_t center = timeToSamples(centerTime);
	int64_t shapeid = center / samplesPerShape;

	m_rectangle.setOrigin(0.5f, 0.5f);

	int64_t left = shapeid - delta;
	int64_t right = shapeid + delta;

	if (left < 0)
		left = 0;
	if ((uint64_t)right >= m_sampleShapes.size())
		right = (int64_t)m_sampleShapes.size() - 1;

	m_rectangle.setPosition(0.0f, 0.0f);
	m_rectangle.setScale(1.0f, m_sampleShapes[shapeid]);
	target.draw(m_rectangle, states);

	float move;
	move = -1.0f;
	for (int64_t i = shapeid - 1; i >= left; --i, move -= 1.0f) {
		m_rectangle.setPosition(move, 0.0f);
		m_rectangle.setScale(1.0f, m_sampleShapes[i]);
		target.draw(m_rectangle, states);
	}
	move = +1.0f;
	for (int64_t i = shapeid + 1; i <= right; ++i, move += 1.0f) {
		m_rectangle.setPosition(move, 0.0f);
		m_rectangle.setScale(1.0f, m_sampleShapes[i]);
		target.draw(m_rectangle, states);
	}

	return;
}

void VisualMusic::computeSamples() {
	m_file.seek(0);

	sf::err() << "Channel Cnt: " << getChannelCount() << std::endl;
	sf::err() << "Sample Rate: " << getSampleRate() << std::endl;

	m_pointsPerShape = getSampleRate() / 96;
	uint32_t samplesPerShape = m_pointsPerShape * getChannelCount();
	uint64_t totalSampleCnt = m_file.getSampleCount();
	uint64_t shapeCnt = totalSampleCnt / samplesPerShape + ((totalSampleCnt % samplesPerShape) ? 1 : 0);

	m_sampleShapes.resize(shapeCnt);
	for (uint64_t i = 0; i < shapeCnt; ++i) {
		uint64_t readcount = m_file.read(m_samples.data(), samplesPerShape);
		double sum = 0.0;
		for (uint64_t j = 0; j < readcount; ++j) {
			sum += std::abs(m_samples[j]);
		}
		sum /= readcount * (32768.0 / 255.0);
		m_sampleShapes[i] = static_cast<uint8_t>(sum);
	}

	return m_file.seek(0);
}
