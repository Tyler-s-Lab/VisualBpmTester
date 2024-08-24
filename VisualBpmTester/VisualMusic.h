#pragma once

#include <SFML/Audio/Music.hpp>
#include <mutex>
#include <queue>
#include <memory>

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

class VisualMusic :
	public sf::SoundStream,
	public sf::Drawable {
	template<typename _Ty>
	using Span = sf::Music::Span<_Ty>;
	using TimeSpan = Span<sf::Time>;

public:
	VisualMusic();
	~VisualMusic();

public:
	bool openFromFile(const std::string& filename);
	bool openFromMemory(const void* data, std::size_t sizeInBytes);
	bool openFromStream(sf::InputStream& stream);

	sf::Time getDuration() const;

	TimeSpan getLoopPoints() const;
	void setLoopPoints(TimeSpan timePoints);

protected:
	virtual bool onGetData(Chunk& data) override;

	virtual void onSeek(sf::Time timeOffset) override;

	virtual sf::Int64 onLoop() override;

private:
	void initialize();

	sf::Uint64 timeToSamples(sf::Time position) const;

	sf::Time samplesToTime(uint64_t samples) const;

private:
	sf::InputSoundFile   m_file;     //!< The streamed music file
	std::vector<int16_t> m_samples;  //!< Temporary buffer of samples
	std::mutex           m_mutex;    //!< Mutex protecting the data
	Span<sf::Uint64>     m_loopSpan; //!< Loop Range Specifier

public:
	void setShowRange(bool followMusic, TimeSpan range = TimeSpan(sf::Time::Zero, sf::Time::Zero));

protected:
	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

	void computeSamples();

private:
	static constexpr uint32_t ShapesPerSec = 96;
	mutable sf::RectangleShape m_rectangle;
	std::vector<std::vector<uint8_t>> m_channelShapes;
};
