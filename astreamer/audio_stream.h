/*
 * This file is part of the FreeStreamer project,
 * (C)Copyright 2011-2014 Matias Muhonen <mmu@iki.fi>
 * See the file ''LICENSE'' for using the code.
 *
 * https://github.com/muhku/FreeStreamer
 */

#ifndef ASTREAMER_AUDIO_STREAM_H
#define ASTREAMER_AUDIO_STREAM_H

#import "input_stream.h"
#include "audio_queue.h"

#include <AudioToolbox/AudioToolbox.h>
#include <list>

namespace astreamer {
    
    typedef struct queued_packet {
        AudioStreamPacketDescription desc;
        struct queued_packet *next;
        char data[];
    } queued_packet_t;
    
    enum Audio_Stream_Error {
        AS_ERR_OPEN = 1,          // Cannot open the audio stream
        AS_ERR_STREAM_PARSE = 2,  // Parse error
        AS_ERR_NETWORK = 3,        // Network error
        AS_ERR_UNSUPPORTED_FORMAT = 4,
        AS_ERR_BOUNCING = 5
    };
    
    class Audio_Stream_Delegate;
    class File_Output;
    
#define kAudioStreamBitrateBufferSize 50
	
    class Audio_Stream : public Input_Stream_Delegate, public Audio_Queue_Delegate {

    public:
        Audio_Stream_Delegate *m_delegate;
        
        enum State {
            STOPPED,
            BUFFERING,
            PLAYING,
            PAUSED,
            SEEKING,
            FAILED,
            END_OF_FILE
        };
        
        Audio_Stream();
        virtual ~Audio_Stream();

        void open();
        void open(Input_Stream_Position *position);
        void close();
        void pause();

        double timePlayedInSeconds();
        double durationInSeconds();
        void seekToTime(double newSeekTime);

        Input_Stream_Position streamPositionForTime(double newSeekTime);
        
        void setVolume(float volume);
        void setPlayRate(float playRate);
        
        void setUrl(CFURLRef url);
        void setStrictContentTypeChecking(bool strictChecking);
        void setDefaultContentType(CFStringRef defaultContentType);
        void setSeekPosition(double seekPosition);
        void setContentLength(UInt64 contentLength);
        
        void setOutputFile(CFURLRef url);
        CFURLRef outputFile();
        
        State state();
        
        CFStringRef sourceFormatDescription();
        CFStringRef contentType();
        
        size_t cachedDataSize();
        
        /* Audio_Queue_Delegate */
        void audioQueueStateChanged(Audio_Queue::State state);
        void audioQueueBuffersEmpty();
        void audioQueueOverflow();
        void audioQueueUnderflow();
        void audioQueueInitializationFailed();
        void audioQueueFinishedPlayingPacket();
        
        /* Input_Stream_Delegate */
        void streamIsReadyRead();
        void streamHasBytesAvailable(UInt8 *data, UInt32 numBytes);
        void streamEndEncountered();
        void streamErrorOccurred();
        void streamMetaDataAvailable(std::map<CFStringRef,CFStringRef> metaData);
        
    private:
        
        Audio_Stream(const Audio_Stream&);
        Audio_Stream &operator = (const Audio_Stream&);

        bool m_inputStreamRunning;
        bool m_audioStreamParserRunning;
        
        UInt64 m_contentLength;
        
        State m_state;
        Input_Stream *m_inputStream;
        Audio_Queue *m_audioQueue;
        
        CFRunLoopTimerRef m_watchdogTimer;
        
        AudioFileStreamID m_audioFileStream;  // the audio file stream parser
        AudioConverterRef m_audioConverter;
        AudioStreamBasicDescription m_srcFormat;
        AudioStreamBasicDescription m_dstFormat;
        OSStatus m_initializationError;
        
        UInt32 m_outputBufferSize;
        UInt8 *m_outputBuffer;
        
        UInt64 m_dataOffset;
        double m_seekPosition;
        size_t m_bounceCount;
        CFAbsoluteTime m_firstBufferingTime;
        
        bool m_strictContentTypeChecking;
        CFStringRef m_defaultContentType;
        CFStringRef m_contentType;
        
        File_Output *m_fileOutput;
        
        CFURLRef m_outputFile;
        
        queued_packet_t *m_queuedHead;
        queued_packet_t *m_queuedTail;
        
        std::list <queued_packet_t*> m_processedPackets;
        
        size_t m_cachedDataSize;
        
        UInt32 m_processedPacketsCount;  // global packet statistics: count
        UInt64 m_audioDataByteCount;
        
        double m_packetDuration;
        double m_bitrateBuffer[kAudioStreamBitrateBufferSize];
        size_t m_bitrateBufferIndex;
        
        float m_outputVolume;
        
        bool m_queueCanAcceptPackets;
        
        Audio_Queue *audioQueue();
        void closeAudioQueue();
        
        UInt64 contentLength();
        void closeAndSignalError(int error);
        void setState(State state);
        void setCookiesForStream(AudioFileStreamID inAudioFileStream);
        double bitrate();
        
        int cachedDataCount();
        void enqueueCachedData(int minPacketsRequired);
        
        static void watchdogTimerCallback(CFRunLoopTimerRef timer, void *info);
        
        static OSStatus encoderDataCallback(AudioConverterRef inAudioConverter,
                                            UInt32 *ioNumberDataPackets,
                                            AudioBufferList *ioData,
                                            AudioStreamPacketDescription **outDataPacketDescription,
                                            void *inUserData);
        static void propertyValueCallback(void *inClientData,
                                          AudioFileStreamID inAudioFileStream,
                                          AudioFileStreamPropertyID inPropertyID,
                                          UInt32 *ioFlags);
        static void streamDataCallback(void *inClientData,
                                       UInt32 inNumberBytes,
                                       UInt32 inNumberPackets,
                                       const void *inInputData,
                                       AudioStreamPacketDescription *inPacketDescriptions);
        AudioFileTypeID audioStreamTypeFromContentType(CFStringRef contentType);
    };
    
    class Audio_Stream_Delegate {
    public:
        virtual void audioStreamStateChanged(Audio_Stream::State state) = 0;
        virtual void audioStreamErrorOccurred(int errorCode) = 0;
        virtual void audioStreamMetaDataAvailable(std::map<CFStringRef,CFStringRef> metaData) = 0;
        virtual void samplesAvailable(AudioBufferList samples, AudioStreamPacketDescription description) = 0;
    };    
    
} // namespace astreamer

#endif // ASTREAMER_AUDIO_STREAM_H
