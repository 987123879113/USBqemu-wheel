//
// Types to shared by platforms and config. dialog.
//

#ifndef AUDIODEFS_H
#define AUDIODEFS_H

#include <string>
#include <vector>
#include <queue>

//TODO sufficient for linux too?
struct AudioDeviceInfoA
{
    //int intID; //optional ID
    std::string strID;
    std::string strName; //gui name
};

struct AudioDeviceInfoW
{
    //int intID; //optional ID
    std::wstring strID;
    std::wstring strName; //gui name
};

#if _WIN32
#define AudioDeviceInfo AudioDeviceInfoW
#else
#define AudioDeviceInfo AudioDeviceInfoA
#endif

class AudioSource
{
public:
	//get buffer converted to 16bit int format
	virtual uint32_t GetBuffer(uint16_t *buff, uint32_t len) = 0;
	//16-bit buffer size
	virtual bool GetBufferSize(uint32_t *size) = 0;
	virtual void SetResampling(int samplerate) = 0;
	virtual uint32_t GetChannels() = 0;

	virtual void Start(){}
	virtual void Stop(){}
};

typedef std::vector<AudioDeviceInfo> AudioDeviceInfoList;

bool AudioInit();
void AudioDeinit();
AudioSource *CreateNewAudioSource(AudioDeviceInfo &dev);
void GetAudioDevices(std::vector<AudioDeviceInfo> &devices);
#endif