#include "common.hpp"

#define BUFFER_COUNT 4
#define BUFFER_SIZE 20480

struct options_t
{
    std::string path;
    float volume;
};

std::unique_ptr<options_t> process_options(int argc, char *argv[])
{
    po::options_description generic("Options");
    generic.add_options()
        ("volume,v", po::value<float>()->default_value(1.0f), "Set playback volume.")
        ("help,h", "Print help message.")
    ;
    po::options_description hidden("Hidden Options");
    hidden.add_options()
        ("path", po::value<std::string>()->default_value("", ""), "Path to audio file.")
    ;
    
    po::options_description all("All Options");
    all.add(generic).add(hidden);
    
    po::positional_options_description p;
    p.add("path", -1);
    
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(all).positional(p).run(), vm);
    po::notify(vm);
    
    std::cout << "Play (v" << PROJECT_VERSION << ") - Command Line Audio Player" << std::endl;
    
    if (vm.count("help"))
    {
        std::cout << "Usage: play [options] [path]" << std::endl;
        std::cout << generic;
        return {};
    }
    
    auto result = std::make_unique<options_t>();
    result->path = vm["path"].as<std::string>();
    result->volume = vm["volume"].as<float>();
    return result;
}

namespace vf {

class AudioDecoder
{
public:
    AudioDecoder(std::string const& path):
        _formatContext{av::FormatContext::Null},
        _audioStream{},
        _audioCodecContext{av::CodecContext::Null}
    {
        _formatContext.open(path);
        _formatContext.maxAnalyzeDuration(1.5);
        _formatContext.findStreamInfo();
        
        _audioStream = av::Stream{_formatContext.findBestStream(av::MediaType::Audio)};
        _audioCodecContext.open(_audioStream);
    }
    
    ~AudioDecoder()
    {
        _audioCodecContext.close();
        
        _formatContext.close();
    }

    inline av::CodecContext& audioCodec()
    {
        return _audioCodecContext;
    }
    
    inline av::CodecContext const& audioCodec() const
    {
        return _audioCodecContext;
    }

    bool readAudioFrame(av::Frame& frame)
    {
        av::Packet packet;
        while (true)
        {
            try
            {
                while (true)
                {
                    _formatContext.readFrame(packet);
                    if (packet.streamIndex() == _audioStream.index())
                    {
                        break;
                    }
                }
            }
            catch (std::runtime_error&)
            {
                return false;
            }
            
            if(_audioCodecContext.decodeAudio(frame, packet))
            {
                return true;
            }
        }
        
        return false;
    }
    
private:
    av::FormatContext _formatContext;
    av::Stream _audioStream;
    av::CodecContext _audioCodecContext;
    
    friend std::ostream& operator<<(std::ostream& os, AudioDecoder const& decoder)
    {
        os << vf::format(
            "Audio: %s (%d, %d)",
            decoder.audioCodec().codec().longName(),
            decoder.audioCodec().channels(),
            decoder.audioCodec().sampleRate()
        ) << std::endl;
        
        return os;
    }
};

al::Format convert(av::SampleFormat format, int channels)
{
    switch (format)
    {
        case av::SampleFormat::U8:
        case av::SampleFormat::U8P:
        {
            switch (channels)
            {
                case 1: return al::Format::MONO8;
                case 2: return al::Format::STEREO8;
                default: break;
            }
            break;
        }
        case av::SampleFormat::S16:
        case av::SampleFormat::S16P:
            switch (channels)
            {
                case 1: return al::Format::MONO16;
                case 2: return al::Format::STEREO16;
                default: break;
            }
            break;
        default: break;
    }
    throw std::runtime_error("Incompatible format.");
}

}
/*
void log_callback(void* ptr, int level, const char* fmt, va_list vl)
{
    static int print_prefix = 0;
    char line[1024];
    av_log_format_line(ptr, level, fmt, vl, line, sizeof(line), &print_prefix);
    std::cout << line << std::endl;
}
*/
void reserve(int nb_samples, uint8_t** data, int* max_nb_samples)
{
    if (*max_nb_samples < nb_samples)
    {
        av_freep(data);
        
        int linesize;
        if (av_samples_alloc(data, &linesize, 2, nb_samples, AV_SAMPLE_FMT_S16, 1) < 0)
        {
            throw std::runtime_error("Failed to allocate buffer.");
        }
        
        *max_nb_samples = nb_samples;
    }
}

void play(options_t const& options)
{
    fs::path path{options.path};
    if (!(fs::exists(path) && fs::is_regular_file(path)))
    {
        throw std::runtime_error(vf::format("invalid path to audio file %s", path));
    }
/*
    av_log_set_level(AV_LOG_ERROR);
    av_log_set_callback(log_callback);
*/
    av_register_all();
    avcodec_register_all();
    
    vf::AudioDecoder decoder{path.string()};
    
    swr::Context ctx{decoder.audioCodec()};
/*
    if (decoder.audioCodec().sampleFormat() == av::SampleFormat::U8P)
    {
        decoder.audioCodec().requestSampleFormat(av::SampleFormat::U8);
    }
    if (decoder.audioCodec().sampleFormat() == av::SampleFormat::S16P)
    {
        decoder.audioCodec().requestSampleFormat(av::SampleFormat::S16);
    }
*/
    al::Device device;
    al::Context context{device};
    al::Context::MakeCurrent(context);
    
    al::util::printErrors();
    
    alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
    alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    alListenerf(AL_GAIN, options.volume);
    
    std::vector<al::Buffer> buffers(BUFFER_COUNT);
    al::Source source;
    
    al::util::printErrors();
/*
    av::Packet packet;
    int buffer_size = AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE;
    uint8_t buffer[buffer_size];
    packet.data(buffer);
    packet.size(buffer_size);
*/
    av::Frame srcFrame;
    av::Frame dstFrame{av::SampleFormat::S16, 2, AVCODEC_MAX_AUDIO_FRAME_SIZE};
    
    auto format = vf::convert(static_cast<av::SampleFormat>(dstFrame.format()), dstFrame.channels());
    
    //int max_nb_samples = 0;
    //uint8_t* data[1] = {nullptr};
    
    for (auto& buffer: buffers)
    {
        if (!decoder.readAudioFrame(srcFrame)) return;
//*/
        auto samples = ctx.convert(srcFrame, dstFrame); dstFrame.sampleRate(srcFrame.sampleRate());
        auto size = av_samples_get_buffer_size(nullptr, 2, samples, AV_SAMPLE_FMT_S16, 1);
        //std::cout << size <<std::endl;
 
        buffer.data(format, dstFrame.data(), size, dstFrame.sampleRate());
/*/
        auto size = decoder.audioCodec().getBufferSize(srcFrame.numberSamples());
        buffer.data(format, srcFrame.data(), size, srcFrame.sampleRate());
//*/
        source.queueBuffer(buffer);
    }
    
    source.play();
    
    al::util::printErrors();

    while (true)
    {
        int num = 0;
        while (true)
        {
			num = source.buffersProcessed();
            if (num > 0) break;
			std::this_thread::yield();
		}
        
        while (num--)
        {
            if (!decoder.readAudioFrame(srcFrame)) return;
        
			auto buffer = source.unqueueBuffer();
//*/
            auto samples = ctx.convert(srcFrame, dstFrame); dstFrame.sampleRate(srcFrame.sampleRate());
            auto size = av_samples_get_buffer_size(nullptr, 2, samples, AV_SAMPLE_FMT_S16, 1);
            //std::cout << size << std::endl;
            
            alBufferData(buffer, static_cast<ALenum>(format), dstFrame.data(), size, dstFrame.sampleRate());
/*/
            auto size = decoder.audioCodec().getBufferSize(srcFrame.numberSamples());
            alBufferData(buffer, static_cast<ALenum>(format), srcFrame.data(), size, srcFrame.sampleRate());
//*/
            
            
            source.queueBuffer(buffer);
        }
        
        if (source.state() != AL_PLAYING)
        {
            source.play();
        }
    }
}

int main(int argc, char *argv[])
{
    try
    {
        if(auto options = process_options(argc, argv))
        {
            play(*options);
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "." << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}