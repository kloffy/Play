#ifndef VF_EXT_AL_HPP_INCLUDED
#define VF_EXT_AL_HPP_INCLUDED

#include "../config.hpp"

#if defined(PLATFORM_APPLE)

#include <OpenAL/al.h>
#include <OpenAL/alc.h>

#endif

namespace vf {
namespace ext {
namespace al {
namespace util {

void printErrors()
{
    for (ALenum error = alGetError(); error != AL_NO_ERROR; error = alGetError())
    {
        std::string msg;
        
        switch(error)
        {
        case AL_INVALID_NAME:
			msg = "INVALID_NAME";
            break;
		case AL_INVALID_ENUM:
			msg = "INVALID_ENUM";
            break;
		case AL_INVALID_VALUE:
			msg = "INVALID_VALUE";
            break;
		case AL_INVALID_OPERATION:
			msg = "INVALID_OPERATION";
            break;
		case AL_OUT_OF_MEMORY:
			msg = "OUT_OF_MEMORY";
            break;
		default:
			printf("Unknown Error");
        }
        
        std::cerr << msg << std::endl;
    }
}

}

enum class Format
{
    MONO8 = AL_FORMAT_MONO8,
    MONO16 = AL_FORMAT_MONO16,
    STEREO8 = AL_FORMAT_STEREO8,
    STEREO16 = AL_FORMAT_STEREO16,
};

class Resource
{
public:
    Resource() = default;
    
    Resource(Resource const& context) = delete;
    Resource& operator=(Resource const& other) = delete;
};

class Device: public Resource
{
    friend class Context;

public:
    Device()
    {
        _device = alcOpenDevice(NULL);
        
        if (_device == nullptr)
        {
            throw std::runtime_error("Failed to initialize al::Device.");
        }
    }
    
    ~Device()
    {
        if (_device != nullptr)
        {
            alcCloseDevice(_device);
        }
    }
    
private:
    ALCdevice* _device;
};

class Context: public Resource
{
public:
    struct NullType {};
    static constexpr NullType Null{};

    static void MakeCurrent(Context const& context)
    {
        alcMakeContextCurrent(context._context);
    }

    Context(NullType):
        _context(nullptr)
    {}
    
    explicit Context(Device const& device):
        _context(nullptr)
    {
        _context = alcCreateContext(device._device, NULL);
        
        if (_context == nullptr)
        {
            throw std::runtime_error("Failed to initialize al::Context.");
        }
    }
    
    ~Context()
    {
        if (_context != nullptr)
        {
            alcDestroyContext(_context);
        }
    }
    
private:
    ALCcontext* _context;
};

class Buffer: public Resource
{
    friend class Source;
    
public:
    Buffer()
    {
        alGenBuffers(1, &_id);
    }
    
    ~Buffer()
    {
        alDeleteBuffers(1, &_id);
    }
    
    inline void data(Format format, ALvoid const* data, ALsizei size, ALsizei freq = 44100)
    {
        alBufferData(_id, static_cast<ALenum>(format), data, size, freq);
    }

private:
    ALuint _id;
};

static_assert(sizeof(Buffer) == sizeof(ALuint), "Wrong size!");

class Source: public Resource
{
public:
    Source()
    {
        alGenSources(1, &_id);
    }
    
    ~Source()
    {
        alDeleteSources(1, &_id);
    }

    inline void buffer(Buffer const& buffer)
    {
        alSourcei(_id, AL_BUFFER, buffer._id);
    }
    
    inline float pitch()
    {
        float value;
        alGetSourcef(_id, AL_PITCH, &value);
        return value;
    }
    
    inline void pitch(float value)
    {
        alSourcef(_id, AL_PITCH, value);
    }
    
    inline float gain()
    {
        float value;
        alGetSourcef(_id, AL_GAIN, &value);
        return value;
    }
    
    inline void gain(float value)
    {
        alSourcef(_id, AL_GAIN, value);
    }
    
    inline bool looping()
    {
        ALint value;
        alGetSourcei(_id, AL_LOOPING, &value);
        return value == AL_TRUE;
    }
    
    inline void looping(bool loop)
    {
        alSourcei(_id, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    }
    
    inline int state()
    {
        ALint value;
        alGetSourcei(_id, AL_SOURCE_STATE, &value);
        return value;
    }
    
    inline void play()
    {
        alSourcePlay(_id);
    }
    
    inline void pause()
    {
        alSourcePause(_id);
    }
    
    inline void stop()
    {
        alSourceStop(_id);
    }
    
    inline int buffersProcessed()
    {
        ALint value;
        alGetSourcei(_id, AL_BUFFERS_PROCESSED, &value);
        return value;
    }
    
    inline void queueBuffer(ALuint buffer)
    {
        alSourceQueueBuffers(_id, 1, &buffer);
    }
    
    inline unsigned int unqueueBuffer()
    {
        ALuint buffer;
        alSourceUnqueueBuffers(_id, 1, &buffer);
        return buffer;
    }
    
    inline void queueBuffer(Buffer const& buffer)
    {
        queueBuffer(buffer._id);
    }
    
private:
    ALuint _id;
};

}
}
}

#endif // VF_EXT_AL_HPP_INCLUDED