#pragma once

#include <SFML/Audio/Music.hpp>
#include <mutex>

class VisualMusic :
	public sf::SoundStream {
	template<typename _Ty>
	using Span = sf::Music::Span<_Ty>;
	using TimeSpan = Span<sf::Time>;

public:
	VisualMusic();
	~VisualMusic();

public:
	bool openFromFile(const std::string& filename);

	sf::Time getDuration() const;

	TimeSpan getLoopPoints() const;
	void setLoopPoints(TimeSpan timePoints);

protected:
	virtual bool onGetData(Chunk& data) override;

	virtual void onSeek(sf::Time timeOffset) override;

	virtual sf::Int64 onLoop() override;

private:
	void initialize();

	uint64_t timeToSamples(sf::Time position) const;

	sf::Time samplesToTime(uint64_t samples) const;

protected:
	sf::InputSoundFile   m_file;     //!< The streamed music file
	std::vector<int16_t> m_samples;  //!< Temporary buffer of samples
	std::mutex           m_mutex;    //!< Mutex protecting the data
	Span<sf::Uint64>     m_loopSpan; //!< Loop Range Specifier
};
