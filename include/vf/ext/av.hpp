#ifndef VF_EXT_AV_HPP_INCLUDED
#define VF_EXT_AV_HPP_INCLUDED

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#if defined(PixelFormat)
#undef PixelFormat
#endif

namespace vf {
namespace ext {
namespace av {

enum class MediaType
{
    Attachment = AVMEDIA_TYPE_ATTACHMENT,
    Audio = AVMEDIA_TYPE_AUDIO,
    Data = AVMEDIA_TYPE_DATA,
    Video = AVMEDIA_TYPE_VIDEO,
    Subtitle = AVMEDIA_TYPE_SUBTITLE,
};

enum class SampleFormat
{
    DBL = AV_SAMPLE_FMT_DBL,
    DBLP = AV_SAMPLE_FMT_DBLP,
    FLT = AV_SAMPLE_FMT_FLT,
    FLTP = AV_SAMPLE_FMT_FLTP,
    S16 = AV_SAMPLE_FMT_S16,
    S16P = AV_SAMPLE_FMT_S16P,
    U8 = AV_SAMPLE_FMT_U8,
    U8P = AV_SAMPLE_FMT_U8P,
};

enum class PixelFormat
{
    RGB24 = PIX_FMT_RGB24,
    RGBA = PIX_FMT_RGBA,
    BGRA = PIX_FMT_BGRA,
    ABGR = PIX_FMT_ABGR,
    ARGB = PIX_FMT_ARGB,
};

class Resource
{
public:
    Resource() = default;

private:
    Resource(Resource const& context) = delete;
    Resource& operator=(Resource const& other) = delete;
};

struct Codec
{
    friend class CodecContext;
    
public:
    static Codec FindEncoder(AVCodecID id)
    {
        if (id == CODEC_ID_NONE)
        {
            throw std::runtime_error("Invalid codec.");
        }
        
        return {avcodec_find_encoder(id)};
    }
    
    static Codec FindDecoder(AVCodecID id)
    {
        if (id == CODEC_ID_NONE)
        {
            throw std::runtime_error("Invalid codec.");
        }
        
        return {avcodec_find_decoder(id)};
    }

    Codec():
        _codec(nullptr)
    {}
    
    ~Codec()
    {
        // Observing only, no ownership
    }

    inline char const* longName() const
    {
        return _codec->long_name;
    }
    
    inline char const* name() const
    {
        return _codec->name;
    }
    
    inline MediaType type() const
    {
        return static_cast<MediaType>(_codec->type);
    }
    
private:
    Codec(AVCodec const* codec):
        _codec(codec)
    {}
    
    AVCodec const* _codec;
};

struct Stream
{
    friend class CodecContext;
    friend class FormatContext;
    
public:
    Stream():
        _stream(nullptr)
    {}
    
    ~Stream()
    {
        // Observing only, no ownership
    }
    
    inline Codec codec() const
    {
        return Codec::FindDecoder(_stream->codec->codec_id);
    }
    
    inline double duration() const
    {
        auto const& tb = _stream->time_base;
        return static_cast<double>(_stream->duration) * tb.num / tb.den;
    }
    
    inline int index() const
    {
        return _stream->index;
    }

private:
    Stream(AVStream* stream):
        _stream(stream)
    {}

    AVStream* _stream;
};

class Frame: public Resource
{
    friend class CodecContext;
    
public:
    Frame():
        _frame(nullptr)
    {
        _frame = avcodec_alloc_frame();
        
        if (_frame == nullptr)
        {
            throw std::runtime_error("Failed to create av::FormatContext.");
        }
    }
    
    Frame(av::SampleFormat format, int channels, int numberSamples):
        Frame()
    {
        auto _format = static_cast<AVSampleFormat>(format);
        _frame->format = _format;
        _frame->channels = channels;
        _frame->nb_samples = numberSamples;
        
        auto buffer_size = av_samples_get_buffer_size(NULL, channels, numberSamples, _format, 0);
        auto buffer = static_cast<uint8_t*>(av_malloc(buffer_size));
        if (avcodec_fill_audio_frame(_frame, channels, _format, buffer, buffer_size, 0) < 0)
        {
            throw std::runtime_error("Failed to create av::Frame.");
        }
    }
    
    Frame(av::PixelFormat format, int width, int height):
        Frame()
    {
        auto _format = static_cast<AVPixelFormat>(format);
        auto num_bytes = avpicture_get_size(_format, width, height);
        auto buffer = static_cast<uint8_t*>(av_malloc(num_bytes*sizeof(uint8_t)));
        if (avpicture_fill((AVPicture*)_frame, buffer, _format, width, height) < 0)
        {
            throw std::runtime_error("Failed to create av::Frame.");
        }
        _frame->format = _format;
        _frame->width = width;
        _frame->height = height;
    }
    
    ~Frame()
    {
        avcodec_free_frame(&_frame);
    }
    
    inline void defaults()
    {
        avcodec_get_frame_defaults(_frame);
    }
    
    inline uint8_t** dataPtr() const
    {
        return _frame->data;
    }
    
    inline uint8_t** extendedDataPtr() const
    {
        return _frame->extended_data;
    }
    
    inline int* lineSizePtr() const
    {
        return _frame->linesize;
    }
    
    inline uint8_t* data(int index = 0) const
    {
        return _frame->data[index];
    }
    
    inline uint8_t* extendedData(int index = 0) const
    {
        return _frame->extended_data[index];
    }
    
    inline int lineSize(int index = 0) const
    {
        return _frame->linesize[index];
    }
    
    inline int width() const
    {
        return _frame->width;
    }
    
    inline int height() const
    {
        return _frame->height;
    }
    
    inline int format() const
    {
        return _frame->format;
    }
    
    inline int channels() const
    {
        return _frame->channels;
    }
    
    inline uint64_t channelLayout() const
    {
        return _frame->channel_layout;
    }
    
    inline int sampleRate() const
    {
        return _frame->sample_rate;
    }
    
    inline void sampleRate(int rate)
    {
        _frame->sample_rate = rate;
    }
    
    inline int numberSamples() const
    {
        return _frame->nb_samples;
    }
    
    inline void numberSamples(int samples)
    {
        _frame->nb_samples = samples;
    }
    
private:
    AVFrame* _frame;
};

class Packet: public Resource
{
    friend class CodecContext;
    friend class FormatContext;

public:
    Packet():
        _packet()
    {
        av_init_packet(&_packet);
    }
    
    ~Packet()
    {
        av_free_packet(&_packet);
    }

    inline uint8_t* data() const
    {
        return _packet.data;
    }
    
    inline uint8_t* data(uint8_t* data)
    {
        return _packet.data = data;
    }
    
    inline int size() const
    {
        return _packet.size;
    }

    inline int size(int size)
    {
        return _packet.size = size;
    }
    
    inline int streamIndex() const
    {
        return _packet.stream_index;
    }
    
private:
    AVPacket _packet;
};

class CodecContext: public Resource
{
public:
    struct NullType {};
    static constexpr NullType Null{};

    CodecContext(NullType):
        _codecContext(nullptr)
    {}
    
    ~CodecContext()
    {
        // Observing only, no ownership
        // Leak?
    }
    
    CodecContext(CodecContext&& other)
    {
        swap(*this, other);
    }
    
    CodecContext& operator=(CodecContext&& other)
    {
        swap(*this, other);
        return *this;
    }
    
    inline Codec codec() const
    {
        return Codec{_codecContext->codec};
    }
    
    inline int width() const
    {
        return _codecContext->width;
    }
    
    inline int height() const
    {
        return _codecContext->height;
    }
    
    inline int pixelFormat() const
    {
        return _codecContext->pix_fmt;
    }
    
    inline int frameSize() const
    {
        return _codecContext->frame_size;
    }
    
    inline int channels() const
    {
        return _codecContext->channels;
    }
    
    inline uint64_t channelLayout() const
    {
        return _codecContext->channel_layout;
    }
    
    inline SampleFormat sampleFormat() const
    {
        return static_cast<SampleFormat>(_codecContext->sample_fmt);
    }
    
    inline int sampleRate() const
    {
        return _codecContext->sample_rate;
    }
    
    inline SampleFormat requestSampleFormat() const
    {
        return static_cast<SampleFormat>(_codecContext->request_sample_fmt);
    }
    
    inline void requestSampleFormat(SampleFormat format)
    {
        _codecContext->request_sample_fmt = static_cast<AVSampleFormat>(format);
    }
    
    inline void open(Stream const& stream)
    {
        auto codec = stream.codec();
    
        _codecContext = stream._stream->codec;
        
        auto result = avcodec_open2(_codecContext, codec._codec, nullptr);
        
        if (result != 0)
        {
            throw std::runtime_error("Failed to initialize av::CodecContext.");
        }
    }
    
    inline void close()
    {
        avcodec_close(_codecContext);
    }
    
    inline int decodeAudio(Frame& frame, Packet const& packet)
    {
        frame.defaults();
        
        auto isFrameAvailable = 0;
        auto result = avcodec_decode_audio4(_codecContext, frame._frame, &isFrameAvailable, &packet._packet);
        
        if (result < 0)
        {
            throw std::runtime_error("Failed to decode audio.");
        }
        
        return isFrameAvailable;
    }
    
    inline int decodeVideo(Frame& frame, Packet const& packet)
    {
        auto isFrameAvailable = 0;
        auto result = avcodec_decode_video2(_codecContext, frame._frame, &isFrameAvailable, &packet._packet);
        
        if (result < 0)
        {
            throw std::runtime_error("Failed to decode video.");
        }
        
        return isFrameAvailable;
    }
    
    inline int getBufferSize(int samples, int align = 0) const
    {
        return av_samples_get_buffer_size(nullptr, _codecContext->channels, samples, _codecContext->sample_fmt, align);
    }
    
    
private:
    CodecContext(AVCodecContext* codecContext):
        _codecContext(codecContext)
    {}
    
    AVCodecContext* _codecContext;
    
    friend void swap(CodecContext& lhs, CodecContext& rhs)
    {
        using std::swap;
        swap(lhs._codecContext, rhs._codecContext);
    }
};

class FormatContext: public Resource
{
public:
    struct NullType {};
    static constexpr NullType Null{};

    static double TimeBaseToSeconds(int timeBase)
    {
        return static_cast<double>(timeBase) * AV_TIME_BASE;
    }
    
    static int SecondsToTimeBase(double seconds)
    {
        return static_cast<int>(AV_TIME_BASE * seconds);
    }

    FormatContext(NullType):
        _formatContext(nullptr)
    {}

    FormatContext():
        _formatContext(nullptr)
    {
        _formatContext = avformat_alloc_context();
        
        if (_formatContext == nullptr)
        {
            throw std::runtime_error("Failed to create av::FormatContext.");
        }
    }
    
    ~FormatContext()
    {
        if (_formatContext != nullptr)
        {
            avformat_free_context(_formatContext);
        }
    }
    
    FormatContext(FormatContext&& other)
    {
        swap(*this, other);
    }
    
    FormatContext& operator=(FormatContext&& other)
    {
        swap(*this, other);
        return *this;
    }
    
    inline double duration() const
    {
        return FormatContext::TimeBaseToSeconds(_formatContext->duration);
    }
    
    inline double maxAnalyzeDuration() const
    {
        return FormatContext::TimeBaseToSeconds(_formatContext->max_analyze_duration);
    }
    
    inline double maxAnalyzeDuration(double maxAnalyzeDuration)
    {
        _formatContext->max_analyze_duration = FormatContext::SecondsToTimeBase(maxAnalyzeDuration);
        return maxAnalyzeDuration;
    }
    
    inline void open(std::string const& path)
    {
        auto result = avformat_open_input(&_formatContext, path.c_str(), NULL, NULL);
        
        if (result != 0)
        {
            throw std::runtime_error("Failed to open input.");
        }
    }
    
    inline void close()
    {
        avformat_close_input(&_formatContext);
    }
    
    inline void readFrame(Packet& packet)
    {
        auto result = av_read_frame(_formatContext, &packet._packet);
        
        if (result < 0)
        {
            throw std::runtime_error("Failed to read frame.");
        }
    }
    
    inline void findStreamInfo()
    {
        auto result = avformat_find_stream_info(_formatContext, nullptr);
        
        if (result < 0)
        {
            throw std::runtime_error("Failed to find stream info.");
        }
    }
    
    inline Stream findBestStream(MediaType mediaType)
    {
        auto result = av_find_best_stream(_formatContext, static_cast<AVMediaType>(mediaType), -1, -1, nullptr, 0);
        
        if (result < 0)
        {
            throw std::runtime_error("Failed to find best stream."); 
        }
        
        return {_formatContext->streams[result]};
    }

private:
    AVFormatContext* _formatContext;
    
    friend void swap(FormatContext& lhs, FormatContext& rhs)
    {
        using std::swap;
        swap(lhs._formatContext, rhs._formatContext);
    }
};

} // av

namespace sws {

class Context
{
public:
    Context(av::Frame const& src, av::Frame const& dst, int flags = SWS_BILINEAR):
        _context{nullptr}
    {
        _context = sws_getContext(
            src.width(), src.height(), static_cast<AVPixelFormat>(src.format()),
            dst.width(), dst.height(), static_cast<AVPixelFormat>(dst.format()),
            flags,
            nullptr, nullptr, nullptr
        );
        
        if (_context == nullptr)
        {
            throw std::runtime_error("Failed to create sws::Context.");
        }
    }
    
    ~Context()
    {
        sws_freeContext(_context);
    }
    
    inline int scale(av::Frame const& src, av::Frame& dst) const
    {
        return sws_scale(
            _context,
            src.dataPtr(), src.lineSizePtr(), 0, src.height(),
            dst.dataPtr(), dst.lineSizePtr()
        );
    }
    
private:
    SwsContext* _context;
};

} // sws

namespace swr {

class Context
{
public:
    Context(av::CodecContext const& codecContext):
        _context(nullptr)
    {
        _context = swr_alloc();
        
        auto layout = codecContext.channelLayout();
        if (!layout)
        {
            layout = av_get_default_channel_layout(codecContext.channels());
        }
        
        av_opt_set_int(_context, "in_channel_layout", layout, 0);
        av_opt_set_int(_context, "out_channel_layout", layout, 0);
        av_opt_set_int(_context, "in_sample_rate", codecContext.sampleRate(), 0);
        av_opt_set_int(_context, "out_sample_rate", codecContext.sampleRate(), 0);
        av_opt_set_sample_fmt(_context, "in_sample_fmt", static_cast<AVSampleFormat>(codecContext.sampleFormat()), 0);
        av_opt_set_sample_fmt(_context, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
        
        swr_init(_context);
    }
    
    ~Context()
    {
        swr_free(&_context);
    }
    
    inline int convert(av::Frame const& src, uint8_t** data, int numberSamples)
    {
        return swr_convert(
            _context,
            data, numberSamples,
            const_cast<uint8_t const**>(src.dataPtr()), src.numberSamples()
        );
    }
    
    inline int convert(av::Frame const& src, av::Frame& dst)
    {
        return swr_convert(
            _context,
            dst.dataPtr(), dst.numberSamples(),
            const_cast<uint8_t const**>(src.dataPtr()), src.numberSamples()
        );
    }
    
private:
    SwrContext* _context;
    
    friend std::ostream& operator<<(std::ostream& os, Context const& context)
    {
        int64_t in_channel_layout;
        int64_t in_sample_rate;
        AVSampleFormat in_sample_fmt;
        
        int64_t out_channel_layout;
        int64_t out_sample_rate;
        AVSampleFormat out_sample_fmt;
        
        av_opt_get_int(context._context, "in_channel_layout", 0, &in_channel_layout);
        av_opt_get_int(context._context, "in_sample_rate", 0, &in_sample_rate);
        av_opt_get_sample_fmt(context._context, "in_sample_fmt", 0, &in_sample_fmt);
        
        av_opt_get_int(context._context, "out_channel_layout", 0, &out_channel_layout);
        av_opt_get_int(context._context, "out_sample_rate", 0, &out_sample_rate);
        av_opt_get_sample_fmt(context._context, "out_sample_fmt", 0, &out_sample_fmt);
        
        os  << "swr::Context {"
            << "(" << in_channel_layout << ", " << in_sample_rate << ", " << in_sample_fmt << ")"
            << "->"
            << "(" << out_channel_layout << ", " << out_sample_rate << ", " << out_sample_fmt << ")"
            << "}";
        
        return os;
    }
};

} // swr
} // ext
} // vf

#endif // VF_EXT_AL_HPP_INCLUDED