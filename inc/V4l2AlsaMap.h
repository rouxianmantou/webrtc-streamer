/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** V4l2AlsaMap.h
**
** -------------------------------------------------------------------------*/

#pragma once

#ifndef WIN32
#include <dirent.h>
#include <stdlib.h>

std::string getUsbPort(const char* fullPath) {
	char* resolvedPath = realpath(fullPath, NULL);
	std::string realPath = "";
	if (resolvedPath) {
		std::cout << "Get real path SUCCESS: " << resolvedPath << std::endl;
		realPath = resolvedPath;
	} else {
		std::cout << "Get real path FAILED!" << std::endl;
		realPath = fullPath;
	}
	auto lastSlashPos = realPath.find_last_of('/');
	std::string lastPath = "";
	if (lastSlashPos != std::string::npos && lastSlashPos < realPath.length() - 1) {
		lastPath = realPath.substr(lastSlashPos + 1);
	} else {
		lastPath = realPath;
	}

	auto colonPos = lastPath.find_first_of(':');
	if (colonPos != std::string::npos) {
		lastPath = lastPath.substr(0, colonPos);
	}

	return lastPath;
}

/* ---------------------------------------------------------------------------
**  get a "deviceid" from uevent sys file
** -------------------------------------------------------------------------*/
std::string getDeviceId(const std::string& evt, const std::string& devicePath) {
    std::string deviceid;
    std::istringstream f(evt);
    std::string key;
	std::string deviceUsbPort = getUsbPort(devicePath.c_str());
    while (getline(f, key, '=')) {
            std::string value;
	    if (getline(f, value)) {
		    if ( (key =="PRODUCT") || (key == "PCI_SUBSYS_ID") ) {
			    value.append("/").append(deviceUsbPort);
				std::cout << "Device ID: " << value << std::endl;
				deviceid = value;
			    break;
		    }
	    }
    }
    return deviceid;
}

std::map<std::string,std::string> getVideoDevices() {
	std::map<std::string,std::string> videodevices;
	std::string video4linuxPath("/sys/class/video4linux");
	DIR *dp = opendir(video4linuxPath.c_str());
	if (dp != NULL) {
		struct dirent *entry = NULL;
		while((entry = readdir(dp))) {
			std::string devicename;
			std::string deviceid;
			std::string deviceSymlinkPath(video4linuxPath);
			if (strstr(entry->d_name,"video") == entry->d_name) {
				std::string devicePath(video4linuxPath);
				devicePath.append("/").append(entry->d_name).append("/name");
				std::ifstream ifsn(devicePath.c_str());
				devicename = std::string(std::istreambuf_iterator<char>{ifsn}, {});
				devicename.erase(devicename.find_last_not_of("\n")+1);

				std::string ueventPath(video4linuxPath);
				ueventPath.append("/").append(entry->d_name).append("/device/uevent");
				deviceSymlinkPath.append("/").append(entry->d_name).append("/device");
				std::cout << "Video Device {" << entry->d_name << "} path: " << deviceSymlinkPath << std::endl;

				std::ifstream ifsd(ueventPath.c_str());
				deviceid = std::string(std::istreambuf_iterator<char>{ifsd}, {});
				deviceid.erase(deviceid.find_last_not_of("\n")+1);
			}

			if (!devicename.empty() && !deviceid.empty()) {
				videodevices[devicename] = getDeviceId(deviceid, deviceSymlinkPath);
			}
		}
		closedir(dp);
	}
	return videodevices;
}

std::map<std::string,std::string> getAudioDevices() {
	std::map<std::string,std::string> audiodevices;
	std::string audioLinuxPath("/sys/class/sound");
	DIR *dp = opendir(audioLinuxPath.c_str());
	if (dp != NULL) {
		struct dirent *entry = NULL;
		while((entry = readdir(dp))) {
			std::string devicename;
			std::string deviceid;
			std::string deviceSymlinkPath(audioLinuxPath);
			if (strstr(entry->d_name,"card") == entry->d_name) {
				std::string devicePath(audioLinuxPath);
				devicePath.append("/").append(entry->d_name).append("/device/uevent");
				deviceSymlinkPath.append("/").append(entry->d_name).append("/device");
				std::cout << "Audio Device {" << entry->d_name << "} path: " << deviceSymlinkPath << std::endl;

				std::ifstream ifs(devicePath.c_str());
				std::string deviceid = std::string(std::istreambuf_iterator<char>{ifs}, {});
				deviceid.erase(deviceid.find_last_not_of("\n")+1);
				deviceid = getDeviceId(deviceid, deviceSymlinkPath);

				if (!deviceid.empty()) {
					if (audiodevices.find(deviceid) == audiodevices.end()) {
						std::string audioname(entry->d_name);
						int deviceNumber = atoi(audioname.substr(strlen("card")).c_str());

						std::string devname = "audiocap://";
						devname += std::to_string(deviceNumber);
						audiodevices[deviceid] = devname;
					}
				}							
			}
		}	
		closedir(dp);	
	}
	return audiodevices;
}

std::map<std::string,std::string>  getV4l2AlsaMap() {
	std::map<std::string,std::string> videoaudiomap;

	std::map<std::string,std::string> videodevices = getVideoDevices();
	std::map<std::string,std::string> audiodevices = getAudioDevices();

	for (auto & id : videodevices) {
		auto audioDevice = audiodevices.find(id.second);
		if (audioDevice != audiodevices.end()) {
			std::cout <<  id.first << "=>" << audioDevice->second << std::endl;
			videoaudiomap[id.first] = audioDevice->second;
		}
	}
	
	return videoaudiomap;
}
#else
std::map<std::string,std::string>  getV4l2AlsaMap() { return std::map<std::string,std::string>(); };
#endif
