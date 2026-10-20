// Device/filesystem support, recent lists, and app/emulator/theme configuration.
// Proven 0x11cdc8-0x129f9c: 29,296 body bytes (56 written functions and an init pair),
// followed by 263 generated inline/template functions (24,420 bytes).
#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <fcntl.h>
#include "Theme.h"
#include "ConfigTypes.h"
#include "MultiDBCachedTextMenu.h"
struct cJSON {
    cJSON *next,*prev,*child;
    int type;
    char *valuestring;
    int valueint;
    double valuedouble;
    char *string;
};
extern "C" {
    cJSON *cJSON_Parse(const char*);
    cJSON *cJSON_GetObjectItem(const cJSON*,const char*);
    int cJSON_GetArraySize(const cJSON*);
    cJSON *cJSON_GetArrayItem(const cJSON*,int);
    void cJSON_Delete(cJSON*);
    char *cJSON_GetStringValue(const cJSON*);
    int cJSON_IsNumber(const cJSON*);
    int cJSON_IsString(const cJSON*);
    cJSON *cJSON_CreateObject();
    cJSON *cJSON_AddStringToObject(cJSON*,const char*,const char*);
    cJSON *cJSON_AddNumberToObject(cJSON*,const char*,double);
    char *cJSON_PrintUnformatted(const cJSON*);
    void cJSON_DeleteItemFromObject(cJSON*,const char*);
    int readWholeFile(const char*,FILE*,char**);
}

static_assert(sizeof(RecentItem)==100, "recent item layout");
static_assert(sizeof(EmuInfo)==0xdc, "emulator layout");
std::vector<std::string> g_platformPaths; // 0x182064
static char g_ioBuffer[128],g_pathBuffer[128]; // 0x182070, 0x1820f0
extern std::vector<EmuInfo> g_romSourcesA,g_romSourcesB;
// 0x11cdc8
void scalePixels(int sx,int sy,int sw,int sh,int dw,int dh,int stride,int padding,int *src,int *dst) {
    int cx,cy;
    unsigned x=sx<<16,y=sy<<16;
    cx=dw;
    cy=dh;
    // Keep the predecrements in the loop conditions: separate decrement
    // statements emit different Boolean tests at -O0.
    unsigned stepX=(sw<<16)/cx,stepY=(sh<<16)/cy;
    int *out=dst;
    do {
        int *row=src+stride*(y>>16);
        cx=dw;
        x=sx<<16;
        do {
            *out++=row[x>>16];
            x=x+stepX;
        }
        while (--cx!=0);
        out+=padding;
        y=y+stepY;
    }
    while (--cy!=0);
}

// 0x11cf40
std::string sdCardSpace() {
    struct statvfs info;
    if (statvfs("/mnt/SDCARD",&info)!=0) {
        printf("statvfs (%s) fail, error is %s","/mnt/SDCARD",strerror(errno));
        return "umounted";
    }
    std::ostringstream stream;
    unsigned long long total;
    total=info.f_bsize*(unsigned long long)info.f_blocks;
    unsigned long long free;
    free=info.f_bsize*(unsigned long long)info.f_bfree;
    unsigned long long used=total-free;
    if (total<0x10000000) {
        printf("totalspace %lld\n",total);
        return "unmounted";
    }
    stream<<(used>>30)<<"G/"<<(total>>30)<<"G";
    return stream.str();
}

// 0x11d198
unsigned sdCardFreeMB() {
    struct statvfs info;
    if (statvfs("/mnt/SDCARD",&info)!=0) {
        printf("statvfs (%s) fail, error is %s","/mnt/SDCARD",strerror(errno));
        return 0;
    }
    unsigned long long total;
    total=info.f_bsize*(unsigned long long)info.f_blocks;
    unsigned long long free;
    free=info.f_bsize*(unsigned long long)info.f_bfree;
    long long used=total-free;
    if ((unsigned long long)total<0x10000000) {
        printf("totalspace %lld\n",total);
        return 0;
    }
    return free>>20;
}

// 0x11d2e4
bool isMounted(const char *path) {
    bool found=false;
    FILE*f=fopen("/proc/mounts","r");
    if (f) {
        while (fgets(g_ioBuffer,128,f)&&found!=true) {
            if (strstr(g_ioBuffer,path)) {
                found=true;
                printf("found :  %s\n",g_ioBuffer);
            }
        }
        fclose(f);
    }
    return found;
}

// 0x11d3c0
bool sdCardMounted() {
    return isMounted("/mnt/SDCARD");
}

// 0x11d3e0
bool moduleLoaded(const char *name) {
    system("lsmod > /tmp/.lsmod.result");
    FILE*f=fopen("/tmp/.lsmod.result","r");
    if (f) {
        bool found=false;
        while (fgets(g_ioBuffer,128,f)&&found!=true) {
            if (strstr(g_ioBuffer,name)) {
                found=true;
                printf("found :  %s\n",g_ioBuffer);
            }
        }
        fclose(f);
        return found;
    }
    return false;
}

// 0x11d4d0
bool isSpinand() {
    return access("/sys/devices/soc/spinand/modalias",0)==0;
}

// 0x11d504
bool fileExists(const char *path,const char *name,int count) {
    bool found=false;
    if (count<1)return found;
    DIR*d;
    if ((d=opendir(path))!=NULL) {
        dirent*entry;
        while ((entry=readdir(d))&&found!=true) {
            if (strncmp(entry->d_name,".",1)==0)continue;
            if (strncmp(entry->d_name,".",1)==0)continue;
            printf("file %s vs name %s\n",entry->d_name,name);
            if (entry->d_type==DT_REG) {
                if (strncasecmp(entry->d_name,name,strlen(name))==0)found=true;
            }
        }
        closedir(d);
    } else printf("fileExists open %s fail\n",path);
    return found;
}

// 0x11d694
unsigned loadPlatformPaths() {
    int i;
    g_platformPaths.clear();
    if (access("/mnt/P2EXT/.dbs",0)!=0)mkdir("/mnt/P2EXT/.dbs",0777);
    FILE*f=fopen("/usr/trimui/res/oplatform.json","r+");
    if (f) {
        int unused1=0,unused2=0;
        char*content;
        std::string path;
        RecentItem unused;
        if (readWholeFile("/usr/trimui/res/oplatform.json",f,&content)>=0) {
            cJSON*root=cJSON_Parse(content);
            if (root) {
                cJSON*array=cJSON_GetObjectItem(root,"list");
                if (array) {
                    int len=cJSON_GetArraySize(array);
                    for (i=0;i<len;i++) {
                        cJSON*entry=cJSON_GetArrayItem(array,i);
                        if (entry) {
                            cJSON*item=cJSON_GetObjectItem(entry,"checkpath");
                            if (item&&item->type==16) {
                                snprintf(g_pathBuffer,128,"%s/%s","/mnt/P2EXT",item->valuestring);
                                if (access(g_pathBuffer,0)==0) {
                                    item=cJSON_GetObjectItem(entry,"mame");
                                    if (item&&item->type==16) {
                                        path="/mnt/P2EXT";
                                        path+="/";
                                        path+=item->valuestring;
                                    } else path="";
                                    g_platformPaths.push_back(path);
                                    item=cJSON_GetObjectItem(entry,"neogeo");
                                    if (item&&item->type==16) {
                                        path="/mnt/P2EXT";
                                        path+="/";
                                        path+=item->valuestring;
                                    } else path="";
                                    g_platformPaths.push_back(path);
                                    item=cJSON_GetObjectItem(entry,"fc");
                                    if (item&&item->type==16) {
                                        path="/mnt/P2EXT";
                                        path+="/";
                                        path+=item->valuestring;
                                    } else path="";
                                    g_platformPaths.push_back(path);
                                    item=cJSON_GetObjectItem(entry,"sfc");
                                    if (item&&item->type==16) {
                                        path="/mnt/P2EXT";
                                        path+="/";
                                        path+=item->valuestring;
                                    } else path="";
                                    g_platformPaths.push_back(path);
                                    item=cJSON_GetObjectItem(entry,"gb");
                                    if (item&&item->type==16) {
                                        path="/mnt/P2EXT";
                                        path+="/";
                                        path+=item->valuestring;
                                    } else path="";
                                    g_platformPaths.push_back(path);
                                    item=cJSON_GetObjectItem(entry,"md");
                                    if (item&&item->type==16) {
                                        path="/mnt/P2EXT";
                                        path+="/";
                                        path+=item->valuestring;
                                    } else path="";
                                    g_platformPaths.push_back(path);
                                    item=cJSON_GetObjectItem(entry,"gba");
                                    if (item&&item->type==16) {
                                        path="/mnt/P2EXT";
                                        path+="/";
                                        path+=item->valuestring;
                                    } else path="";
                                    g_platformPaths.push_back(path);
                                    item=cJSON_GetObjectItem(entry,"ms");
                                    if (item&&item->type==16) {
                                        path="/mnt/P2EXT";
                                        path+="/";
                                        path+=item->valuestring;
                                    } else path="";
                                    g_platformPaths.push_back(path);
                                    item=cJSON_GetObjectItem(entry,"pce");
                                    if (item&&item->type==16) {
                                        path="/mnt/P2EXT";
                                        path+="/";
                                        path+=item->valuestring;
                                    } else path="";
                                    g_platformPaths.push_back(path);
                                    item=cJSON_GetObjectItem(entry,"ps");
                                    if (item&&item->type==16) {
                                        path="/mnt/P2EXT";
                                        path+="/";
                                        path+=item->valuestring;
                                    } else path="";
                                    g_platformPaths.push_back(path);
                                } else printf("checkpath %s not exists\n",g_pathBuffer);
                            }
                        }
                    }
                }
                cJSON_Delete(root);
            } else printf("parse fail : %s\n",content);
            free(content);
        } else printf("fail to load %s\n","/usr/trimui/res/oplatform.json");
        fclose(f);
    } else printf("fail to open %s\n","/usr/trimui/res/oplatform.json");
    return g_platformPaths.size();
}

// This first use follows RecentItem construction: moving the declaration earlier
// changes the inline constructor/destructor emission order.
std::list<RecentItem> g_recentList;
// 0x182170
// 0x11dfa0
bool dualPartition() {
    if (access("/sys/block/mmcblk0/mmcblk0p2",0)==0&&access("/sys/block/mmcblk0/mmcblk0p1",0)==0) {
        if (fileExists("/mnt/SDCARD","KERNEL",1))return 1;
    }
    return 0;
}

// 0x11e02c
int copyNeoGeo(const char *path) {
    snprintf(g_ioBuffer,128,"%s/neogeo.zip",path);
    if (access(g_ioBuffer,0)!=0) {
        if (access("/mnt/P2EXT/system/neogeo.zip",0)==0) {
            snprintf(g_ioBuffer,128,"cp /mnt/P2EXT/system/neogeo.zip %s/",path);
            return system(g_ioBuffer);
        }
    }
    return -1;
}

// 0x11e0f0
const char *platformPath(int index) {
    if (dualPartition()!=true)return NULL;
    if (isMounted("/mnt/P2EXT")) {
        if (index>=0&&index<g_platformPaths.size()) {
            if (g_platformPaths[index].length()!=0) {
                printf("get xml %d: %s\n",index,g_platformPaths[index].c_str());
                return g_platformPaths[index].c_str();
            }
        }
    }
    return NULL;
}

// 0x11e220
bool verifyGameList(const char*file,const char*sha) {
    snprintf(g_ioBuffer,128,"sha256sum %s > /tmp/.gamelist_sha256",file);
    int rc=system(g_ioBuffer);
    snprintf(g_ioBuffer,128,"diff /tmp/.gamelist_sha256 %s",sha);
    return system(g_ioBuffer)==0;
}

// 0x11e2ac
int mountP2Ext() {
    bool mounted=isMounted("/mnt/P2EXT");
    if (mounted!=true) {
        printf("%s not mounted\n","/mnt/P2EXT");
        int rc=system("/usr/sbin/tune2fs -O ^has_journal /dev/mmcblk0p2");
        if (access("/mnt/P2EXT",0)!=0)mkdir("/mnt/P2EXT",0777);
        rc=system("mount -t ext4 /dev/mmcblk0p2 /mnt/P2EXT");
        return isMounted("/mnt/P2EXT");
    } else {
        printf("%s mounted\n","/mnt/P2EXT");
        return 1;
    }
}

// 0x11e390
int removeAllDBFiles() {
    puts("removeAllDBFiles");
    if (dualPartition()!=true) {
        int suffix;
        unsigned j,k;
        int rc;
        std::string cmd;
        for (suffix=0;suffix<7;suffix++) {
            {
                std::stringstream stream(std::ios::out|std::ios::in);
                stream<<suffix;
                cmd="rm -f ";
                cmd+="/mnt/SDCARD/Roms/ARCADE/";
                cmd+="/*_cache";
                if (suffix>0)cmd+=stream.str()+".db";
                else cmd+=".db";
                rc=system(cmd.c_str());
                printf("removedbfiles %s\n",cmd.c_str());
            }
            {
                std::stringstream stream(std::ios::out|std::ios::in);
                stream<<suffix;
                cmd="rm -f ";
                cmd+="/mnt/SDCARD/Roms/NEOGEO/";
                cmd+="/*_cache";
                if (suffix>0)cmd+=stream.str()+".db";
                else cmd+=".db";
                rc=system(cmd.c_str());
                printf("removedbfiles %s\n",cmd.c_str());
            }
            {
                std::stringstream stream(std::ios::out|std::ios::in);
                stream<<suffix;
                cmd="rm -f ";
                cmd+="/mnt/SDCARD/Roms/FC/";
                cmd+="/*_cache";
                if (suffix>0)cmd+=stream.str()+".db";
                else cmd+=".db";
                rc=system(cmd.c_str());
                printf("removedbfiles %s\n",cmd.c_str());
            }
            {
                std::stringstream stream(std::ios::out|std::ios::in);
                stream<<suffix;
                cmd="rm -f ";
                cmd+="/mnt/SDCARD/Roms/SFC/";
                cmd+="/*_cache";
                if (suffix>0)cmd+=stream.str()+".db";
                else cmd+=".db";
                rc=system(cmd.c_str());
                printf("removedbfiles %s\n",cmd.c_str());
            }
            {
                std::stringstream stream(std::ios::out|std::ios::in);
                stream<<suffix;
                cmd="rm -f ";
                cmd+="/mnt/SDCARD/Roms/GB/";
                cmd+="/*_cache";
                if (suffix>0)cmd+=stream.str()+".db";
                else cmd+=".db";
                rc=system(cmd.c_str());
                printf("removedbfiles %s\n",cmd.c_str());
            }
            {
                std::stringstream stream(std::ios::out|std::ios::in);
                stream<<suffix;
                cmd="rm -f ";
                cmd+="/mnt/SDCARD/Roms/MD/";
                cmd+="/*_cache";
                if (suffix>0)cmd+=stream.str()+".db";
                else cmd+=".db";
                rc=system(cmd.c_str());
                printf("removedbfiles %s\n",cmd.c_str());
            }
            {
                std::stringstream stream(std::ios::out|std::ios::in);
                stream<<suffix;
                cmd="rm -f ";
                cmd+="/mnt/SDCARD/Roms/GBA/";
                cmd+="/*_cache";
                if (suffix>0)cmd+=stream.str()+".db";
                else cmd+=".db";
                rc=system(cmd.c_str());
                printf("removedbfiles %s\n",cmd.c_str());
            }
            {
                std::stringstream stream(std::ios::out|std::ios::in);
                stream<<suffix;
                cmd="rm -f ";
                cmd+="/mnt/SDCARD/Roms/MS/";
                cmd+="/*_cache";
                if (suffix>0)cmd+=stream.str()+".db";
                else cmd+=".db";
                rc=system(cmd.c_str());
                printf("removedbfiles %s\n",cmd.c_str());
            }
            {
                std::stringstream stream(std::ios::out|std::ios::in);
                stream<<suffix;
                cmd="rm -f ";
                cmd+="/mnt/SDCARD/Roms/PCE/";
                cmd+="/*_cache";
                if (suffix>0)cmd+=stream.str()+".db";
                else cmd+=".db";
                rc=system(cmd.c_str());
                printf("removedbfiles %s\n",cmd.c_str());
            }
            {
                std::stringstream stream(std::ios::out|std::ios::in);
                stream<<suffix;
                cmd="rm -f ";
                cmd+="/mnt/SDCARD/Roms/PS";
                cmd+="/*_cache";
                if (suffix>0)cmd+=stream.str()+".db";
                else cmd+=".db";
                rc=system(cmd.c_str());
                printf("removedbfiles %s\n",cmd.c_str());
            }
            {
                std::stringstream stream(std::ios::out|std::ios::in);
                stream<<suffix;
                cmd="rm -f ";
                cmd+="/mnt/SDCARD/Roms/OTHER";
                cmd+="/*_cache";
                if (suffix>0)cmd+=stream.str()+".db";
                else cmd+=".db";
                rc=system(cmd.c_str());
                printf("removedbfiles %s\n",cmd.c_str());
            }
            for (j=0;j<g_romSourcesA.size();j++) {
                std::stringstream stream(std::ios::out|std::ios::in);
                stream<<suffix;
                cmd="rm -f ";
                cmd+=g_romSourcesA[j].path;
                cmd+="/*_cache";
                if (suffix>0)cmd+=stream.str()+".db";
                else cmd+=".db";
                rc=system(cmd.c_str());
                printf("removedbfiles %s\n",cmd.c_str());
            }
            for (k=0;k<g_romSourcesB.size();k++) {
                std::stringstream stream(std::ios::out|std::ios::in);
                stream<<suffix;
                cmd="rm -f ";
                cmd+=g_romSourcesB[k].path;
                cmd+="/*_cache";
                if (suffix>0)cmd+=stream.str()+".db";
                else cmd+=".db";
                rc=system(cmd.c_str());
                printf("removedbfiles %s\n",cmd.c_str());
            }
        }
    } else {
        int rc=system("rm -rf /mnt/P2EXT/.dbs/*");
    }
    sync();
    return 1;
}

// 0x11f734
int removeEmulatorConfigs() {
    int rc=system("rm -f /mnt/SDCARD/Roms/ARCADE//.mame4all/configs/*.cfg");
    rc=system("rm -f /mnt/SDCARD/Roms/FC//.fceux/fceux.cfg");
    rc=system("rm -f /mnt/SDCARD/Roms/FC//.fceux/cfg/*.cfg");
    sync();
    return 1;
}

// 0x11f790
int initSdCardFolders() {
    if (dualPartition()!=0)return -1;
    if (access("/mnt/SDCARD/App",0)!=0)mkdir("/mnt/SDCARD/App",0777);
    if (access("/mnt/SDCARD/Roms",0)!=0)mkdir("/mnt/SDCARD/Roms",0777);
    if (access("/mnt/SDCARD/Imgs",0)!=0)mkdir("/mnt/SDCARD/Imgs",0777);
    if (access("/mnt/SDCARD/Roms/ARCADE/",0)!=0)mkdir("/mnt/SDCARD/Roms/ARCADE/",0777);
    if (access("/mnt/SDCARD/Roms/ARCADE//COPYING",0)!=0) {
        int rc1=system("cp /usr/trimui/res/licenses/mame/COPYING /mnt/SDCARD/Roms/ARCADE/");
    }
    if (access("/mnt/SDCARD/Roms/NEOGEO/",0)!=0)mkdir("/mnt/SDCARD/Roms/NEOGEO/",0777);
    if (access("/mnt/SDCARD/Roms/NEOGEO//COPYING",0)!=0) {
        int rc2=system("cp /usr/trimui/res/licenses/mame/COPYING /mnt/SDCARD/Roms/NEOGEO/");
    }
    if (access("/mnt/SDCARD/Roms/FC/",0)!=0)mkdir("/mnt/SDCARD/Roms/FC/",0777);
    if (access("/mnt/SDCARD/Roms/FC//COPYING",0)!=0) {
        int rc3=system("cp /usr/trimui/res/licenses/fceux/COPYING /mnt/SDCARD/Roms/FC/");
    }
    if (access("/mnt/SDCARD/Roms/SFC/",0)!=0)mkdir("/mnt/SDCARD/Roms/SFC/",0777);
    if (access("/mnt/SDCARD/Roms/SFC//LICENSE",0)!=0) {
        int rc4=system("cp /usr/trimui/res/licenses/bsnes/LICENSE /mnt/SDCARD/Roms/SFC/");
    }
    if (access("/mnt/SDCARD/Roms/GB/",0)!=0)mkdir("/mnt/SDCARD/Roms/GB/",0777);
    if (access("/mnt/SDCARD/Roms/GB//COPYING",0)!=0) {
        int rc5=system("cp /usr/trimui/res/licenses/gamebatte/COPYING /mnt/SDCARD/Roms/GB/");
    }
    if (access("/mnt/SDCARD/Roms/MD/",0)!=0)mkdir("/mnt/SDCARD/Roms/MD/",0777);
    if (access("/mnt/SDCARD/Roms/MD//COPYING",0)!=0) {
        int rc6=system("cp /usr/trimui/res/licenses/picodrive/COPYING /mnt/SDCARD/Roms/MD/");
    }
    if (access("/mnt/SDCARD/Roms/GBA/",0)!=0)mkdir("/mnt/SDCARD/Roms/GBA/",0777);
    if (access("/mnt/SDCARD/Roms/GBA//COPYING.DOC",0)!=0) {
        int rc7=system("cp /usr/trimui/res/licenses/gPsp/COPYING.DOC /mnt/SDCARD/Roms/GBA/");
    }
    if (access("/mnt/SDCARD/Roms/MS/",0)!=0)mkdir("/mnt/SDCARD/Roms/MS/",0777);
    if (access("/mnt/SDCARD/Roms/MS//license",0)!=0) {
        int rc8=system("cp /usr/trimui/res/licenses/sms/license /mnt/SDCARD/Roms/MS/");
    }
    if (access("/mnt/SDCARD/Roms/PCE/",0)!=0)mkdir("/mnt/SDCARD/Roms/PCE/",0777);
    if (access("/mnt/SDCARD/Roms/PCE//COPYING",0)!=0) {
        int rc9=system("cp /usr/trimui/res/licenses/temper/COPYING /mnt/SDCARD/Roms/PCE/");
    }
    if (access("/mnt/SDCARD/Roms/PS",0)!=0)mkdir("/mnt/SDCARD/Roms/PS",0777);
    if (access("/mnt/SDCARD/Roms/PS/COPYING",0)!=0) {
        int rc10=system("cp /usr/trimui/res/licenses/pcsx_rearmed/COPYING /mnt/SDCARD/Roms/PS");
    }
    if (access("/mnt/SDCARD/Imgs/ARCADE/",0)!=0)mkdir("/mnt/SDCARD/Imgs/ARCADE/",0777);
    if (access("/mnt/SDCARD/Imgs/NEOGEO/",0)!=0)mkdir("/mnt/SDCARD/Imgs/NEOGEO/",0777);
    if (access("/mnt/SDCARD/Imgs/FC/",0)!=0)mkdir("/mnt/SDCARD/Imgs/FC/",0777);
    if (access("/mnt/SDCARD/Imgs/SFC/",0)!=0)mkdir("/mnt/SDCARD/Imgs/SFC/",0777);
    if (access("/mnt/SDCARD/Imgs/GB/",0)!=0)mkdir("/mnt/SDCARD/Imgs/GB/",0777);
    if (access("/mnt/SDCARD/Imgs/MD/",0)!=0)mkdir("/mnt/SDCARD/Imgs/MD/",0777);
    if (access("/mnt/SDCARD/Imgs/GBA/",0)!=0)mkdir("/mnt/SDCARD/Imgs/GBA/",0777);
    if (access("/mnt/SDCARD/Imgs/MS/",0)!=0)mkdir("/mnt/SDCARD/Imgs/MS/",0777);
    if (access("/mnt/SDCARD/Imgs/PCE/",0)!=0)mkdir("/mnt/SDCARD/Imgs/PCE/",0777);
    if (access("/mnt/SDCARD/Imgs/PS",0)!=0)mkdir("/mnt/SDCARD/Imgs/PS",0777);
    return 0;
}

// 0x11ffa8
int swapSize() {
    int size=0;
    FILE*f=fopen("/proc/swaps","r");
    if (f) {
        fgets(g_ioBuffer,128,f);
        if (fscanf(f,"%s %*s %d %*d %*d",g_ioBuffer,&size)==2)printf("swap file %s\n",g_ioBuffer);
        fclose(f);
    }
    return size;
}

// 0x12005c
int enableSwap() {
    struct stat info;
    int rc;
    if (!(stat("/mnt/SDCARD/cachefile",&info)==0&&S_ISREG(info.st_mode)&&info.st_size==0x8000000)) {
        if (access("/mnt/SDCARD/cachefile",0)==0)remove("/mnt/SDCARD/cachefile");
        rc=system("dd if=/dev/zero of=/mnt/SDCARD/cachefile bs=1M count=128;chmod 600 /mnt/SDCARD/cachefile;mkswap /mnt/SDCARD/cachefile");
    }
    if (access("/mnt/SDCARD/cachefile",0)==0)return system("swapon /mnt/SDCARD/cachefile");
    else return -1;
}

// 0x120154
float maxCpuMHz() {
    int freq=0;
    FILE*f=fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq","r");
    if (f) {
        fscanf(f,"%d",&freq);
        fclose(f);
    }
    return(float)(freq/1000);
}

// 0x1201e0
int memoryMB() {
    int mem=0;
    FILE*f=fopen("/proc/meminfo","r");
    if (f) {
        int rc=fscanf(f,"%*s %d",&mem);
        fclose(f);
        mem=((mem/1024+32)/32)*32;
    }
    return mem;
}

// 0x120290
void removeMax(std::list<int>*list) {
    std::list<int>::iterator it,chosen;
    it=list->begin();
    chosen=it;
    int max=-1;
    while (it!=list->end()) {
        if (max<*it) {
            max=*it;
            chosen=it;
        }
        it++;
    }
    if (max>0)list->erase(chosen);
}

// 0x1203a4
void removeMin(std::list<int>*list) {
    std::list<int>::iterator it,chosen;
    it=list->begin();
    chosen=it;
    int min=100000000;
    while (it!=list->end()) {
        if (*it<min) {
            min=*it;
            chosen=it;
        }
        it++;
    }
    if (min<100000000)list->erase(chosen);
}

// 0x1204c4
int addRecent(std::list<RecentItem>*list,const char*label,const char*launch,const char*rompath,const char*imgpath,int type,bool front) {
    if (!list||!label||!launch)return -1;
    RecentItem item;
    item.label=label;
    item.launch=launch;
    if (rompath)item.rompath=rompath;
    if (imgpath)item.imgpath=imgpath;
    item.type=type;
    std::list<RecentItem>::iterator it;
    for (it=list->begin();it!=list->end();it++) {
        if (strcmp(label,it->label.c_str())==0&&rompath&&strcmp(rompath,it->rompath.c_str())==0&&type==it->type) {
            list->erase(it);
            break;
        }
    }
    if (list->size()>19) {
        if (front)list->pop_back();
        else list->pop_front();
    }
    if (list->size()<20) {
        if (front)list->push_front(item);
        else list->push_back(item);
        printf("Add2RecentList %s %s %d\n",label,rompath,type);
    }
    return 1;
}

// 0x120798
int removeRecent(std::list<RecentItem>*list,const char*label) {
    if (!list)return -1;
    std::list<RecentItem>::iterator it;
    for (it=list->begin();it!=list->end();it++) {
        if (strcmp(label,it->rompath.c_str())==0) {
            list->erase(it);
            return 1;
        }
    }
    return 0;
}

// 0x1208a0
int loadRecentList(std::list<RecentItem>*list) {
    if (!list)return -1;
    FILE*f=fopen("/mnt/SDCARD/Roms/recentlist.json","r+");
    if (f) {
        char*buf=(char*)malloc(0x2000);
        char*label=NULL,*rompath=NULL,*launch=NULL,*imgpath=NULL;
        int type=0;
        RecentItem unused;
        while (fgets(buf,0x2000,f)) {
            cJSON*root=cJSON_Parse(buf);
            if (root) {
                cJSON*item=cJSON_GetObjectItem(root,"label");
                label=cJSON_GetStringValue(item);
                item=cJSON_GetObjectItem(root,"rompath");
                if (item)rompath=cJSON_GetStringValue(item);
                item=cJSON_GetObjectItem(root,"launch");
                launch=cJSON_GetStringValue(item);
                item=cJSON_GetObjectItem(root,"type");
                if (item&&item->type==8)type=item->valueint;
                item=cJSON_GetObjectItem(root,"imgpath");
                if (item)imgpath=cJSON_GetStringValue(item);
                addRecent(list,label,launch,rompath,imgpath,type,false);
                cJSON_Delete(root);
            }
        }
        fclose(f);
        free(buf);
    }
    return list->size();
}

// 0x120af4
int saveRecentList(std::list<RecentItem>*list) {
    std::list<RecentItem>::iterator it;
    FILE*f=fopen("/mnt/SDCARD/Roms/recentlist.json","w+");
    if (f) {
        cJSON*root=cJSON_CreateObject();
        if (root) {
            for (it=list->begin();it!=list->end();it++) {
                cJSON_AddStringToObject(root,"label",it->label.c_str());
                if (it->rompath.size()!=0)cJSON_AddStringToObject(root,"rompath",it->rompath.c_str());
                if (it->imgpath.size()!=0)cJSON_AddStringToObject(root,"imgpath",it->imgpath.c_str());
                cJSON_AddStringToObject(root,"launch",it->launch.c_str());
                cJSON_AddNumberToObject(root,"type",it->type);
                char*text=cJSON_PrintUnformatted(root);
                fprintf(f,"%s\n",text);
                free(text);
                cJSON_DeleteItemFromObject(root,"label");
                if (it->rompath.size()!=0)cJSON_DeleteItemFromObject(root,"rompath");
                if (it->imgpath.size()!=0)cJSON_DeleteItemFromObject(root,"imgpath");
                cJSON_DeleteItemFromObject(root,"launch");
                cJSON_DeleteItemFromObject(root,"type");
            }
            cJSON_Delete(root);
        }
        fclose(f);
        sync();
    }
    return 0;
}

// 0x120e20
int clearRecent(std::list<RecentItem>*list) {
    list->clear();
    unlink("/mnt/SDCARD/Roms/recentlist.json");
    sync();
    return 0;
}

// 0x120e58
static_assert(sizeof(AppInfo)==128, "app layout");
int removeApp(std::vector<AppInfo>*list,const std::string&launch) {
    for (int i=0;i<list->size();i++) {
        if (list->at(i).launch.compare(launch)==0) {
            printf("remove app %s\n",list->at(i).launch.c_str());
            list->erase(list->begin()+i);
            return 0;
        }
    }
    return -1;
}

// 0x120f80
int findEmuByLaunch(std::vector<EmuInfo>*list,const char*path) {
    if (!path)return -1;
    {
        for (int i=0;i<list->size();i++) {
            if (strncmp((*list)[i].launch.c_str(),path,(*list)[i].launch.length())==0)return i;
        }
    }
    return -1;
}

// 0x121074
int findEmuByLabelPrefix(std::vector<EmuInfo>*list,const char*path) {
    if (!path)return -1;
    {
        for (int i=0;i<list->size();i++) {
            if (strncmp((*list)[i].label.c_str(),path,strlen(path))==0)return i;
        }
    }
    return -1;
}

// 0x12114c
int findApp(std::vector<AppInfo>*list,const char*path) {
    for (int i=0;i<list->size();i++) {
        if (strncmp((*list)[i].launch.c_str(),path,(*list)[i].launch.length())==0)return i;
    }
    return -1;
}

// 0x12122c
void parseApp(void*data,const char*base,const char*dir,cJSON*root) {
    std::vector<AppInfo>*list=(std::vector<AppInfo>*)data;
    cJSON*item=NULL;
    char*icon=NULL,*iconSelected=NULL,*launch=NULL,*description=NULL,*label=NULL;
    int autoFlip=0,noAudioFix=0;
    AppInfo app;
    item=cJSON_GetObjectItem(root,"label");
    label=cJSON_GetStringValue(item);
    item=cJSON_GetObjectItem(root,"icon");
    icon=cJSON_GetStringValue(item);
    item=cJSON_GetObjectItem(root,"iconsel");
    if (item)iconSelected=cJSON_GetStringValue(item);
    item=cJSON_GetObjectItem(root,"launch");
    launch=cJSON_GetStringValue(item);
    item=cJSON_GetObjectItem(root,"description");
    if (item)description=cJSON_GetStringValue(item);
    item=cJSON_GetObjectItem(root,"autoflip");
    if (item&&cJSON_IsNumber(item))autoFlip=item->valueint;
    item=cJSON_GetObjectItem(root,"noaudiofix");
    if (item&&cJSON_IsNumber(item))noAudioFix=item->valueint;
    if (icon&&launch&&label) {
        app.label=label;
        if (*icon=='/')app.icon=icon;
        else {
            snprintf(g_ioBuffer,128,"%s/%s/%s",base,dir,icon);
            app.icon=g_ioBuffer;
        }
        if (iconSelected&&strnlen(iconSelected,128)!=0) {
            if (*iconSelected=='/')app.iconSelected=iconSelected;
            else {
                snprintf(g_ioBuffer,128,"%s/%s/%s",base,dir,iconSelected);
                app.iconSelected=g_ioBuffer;
            }
        } else app.iconSelected=app.icon;
        app.launch=launch;
        if (*launch=='/')app.launch=launch;
        else {
            snprintf(g_ioBuffer,128,"%s/%s/%s",base,dir,launch);
            app.launch=g_ioBuffer;
        }
        app.description=description;
        app.autoFlip=autoFlip;
        app.noAudioFix=noAudioFix;
        removeApp(list,app.launch);
        printf("add %s icon(%s) iconsel(%s) launch(%s) autoflip(%d) noaudiofix(%d)\n",app.label.c_str(),app.icon.c_str(),app.iconSelected.c_str(),app.launch.c_str(),app.autoFlip,app.noAudioFix);
        list->push_back(app);
    }
}

// 0x1216e4
int loadConfigFolder(void*data,const char*base,const char*filename,void(*parse)(void*,const char*,const char*,cJSON*)) {
    char*path=(char*)malloc(1024);
    int result=0;
    struct stat info;
    char*content=NULL;
    if (!path)return -1;
    {
        printf("open %s\n",base);
        DIR*d;
        if ((d=opendir(base))!=NULL) {
            dirent*entry;
            while ((entry=readdir(d))!=NULL) {
                if (strncmp(entry->d_name,".",1)!=0&&strncmp(entry->d_name,"..",2)!=0) {
                    snprintf(path,1024,"%s/%s/%s",base,entry->d_name,filename);
                    if (stat(path,&info)==0) {
                        printf("parse %s\n",path);
                        FILE*f=fopen(path,"rb");
                        if (f) {
                            if (readWholeFile(path,f,&content)>=0) {
                                cJSON*root=cJSON_Parse(content);
                                if (root) {
                                    parse(data,base,entry->d_name,root);
                                    cJSON_Delete(root);
                                }
                                free(content);
                            } else printf("load %s fail\n",path);
                            fclose(f);
                        } else printf("open %s fail\n",path);
                    } else printf("stat %s fail\n",path);
                }
            }
            closedir(d);
        }
        free(path);
    }
    return result;
}

// 0x121958
int loadAppFolder(std::vector<AppInfo>*list,const char*base) {
    return loadConfigFolder(list,base,"config.json",parseApp);
}

// 0x121998
int removeEmu(std::vector<EmuInfo>*list,const std::string&launch) {
    for (int i=0;i<list->size();i++) {
        if (list->at(i).launch.compare(launch)==0) {
            printf("remove app %s\n",list->at(i).launch.c_str());
            list->erase(list->begin()+i);
            return 0;
        }
    }
    return -1;
}

// 0x121ac0
void parseEmu(void*data,const char*base,const char*dir,cJSON*root) {
    std::vector<EmuInfo>*list=(std::vector<EmuInfo>*)data;
    cJSON*item=NULL;
    char*label=NULL,*icon=NULL,*iconSelected=NULL,*launch=NULL,*rompath=NULL,*imgpath=NULL,*extlist=NULL,*gamelist=NULL,*verbose=NULL;
    EmuInfo emu;
    item=cJSON_GetObjectItem(root,"label");
    label=cJSON_GetStringValue(item);
    item=cJSON_GetObjectItem(root,"labelverbose");
    if (item) {
        verbose=cJSON_GetStringValue(item);
        if (verbose)emu.labelVerbose=verbose;
    }
    item=cJSON_GetObjectItem(root,"icon");
    icon=cJSON_GetStringValue(item);
    item=cJSON_GetObjectItem(root,"iconsel");
    if (item)iconSelected=cJSON_GetStringValue(item);
    item=cJSON_GetObjectItem(root,"launch");
    launch=cJSON_GetStringValue(item);
    item=cJSON_GetObjectItem(root,"rompath");
    rompath=cJSON_GetStringValue(item);
    item=cJSON_GetObjectItem(root,"imgpath");
    imgpath=cJSON_GetStringValue(item);
    item=cJSON_GetObjectItem(root,"gamelist");
    if (item)gamelist=cJSON_GetStringValue(item);
    item=cJSON_GetObjectItem(root,"useswap");
    if (cJSON_IsNumber(item))emu.useSwap=item->valueint!=0;
    else emu.useSwap=1;
    item=cJSON_GetObjectItem(root,"shortname");
    if (cJSON_IsNumber(item))emu.hasCpinyin=item->valueint!=0;
    else emu.hasCpinyin=0;
    item=cJSON_GetObjectItem(root,"extlist");
    extlist=cJSON_GetStringValue(item);
    if (extlist) {
        std::string extension;
        char*token=NULL;
        token=strtok(extlist,"|");
        while (token) {
            extension=token;
            emu.extensions.push_back(std::string(token));
            token=strtok(NULL,"|");
        }
    }
    item=cJSON_GetObjectItem(root,"hidebios");
    if (cJSON_IsNumber(item))emu.hideBios=item->valueint!=0;
    else emu.hideBios=0;
    emu.noAudioFix=0;
    item=cJSON_GetObjectItem(root,"noaudiofix");
    if (item&&cJSON_IsNumber(item))emu.noAudioFix=item->valueint;
    if (label&&icon&&launch&&((rompath&&imgpath)||gamelist)) {
        emu.label=label;
        if (gamelist) {
            if (*gamelist=='/')emu.gameList=gamelist;
            else {
                snprintf(g_ioBuffer,128,"%s/%s/%s",base,dir,gamelist);
                emu.gameList=g_ioBuffer;
            }
        }
        if (*icon=='/')emu.icon=icon;
        else {
            snprintf(g_ioBuffer,128,"%s/%s/%s",base,dir,icon);
            emu.icon=g_ioBuffer;
        }
        if (iconSelected&&strnlen(iconSelected,128)!=0) {
            if (*iconSelected=='/')emu.iconSelected=iconSelected;
            else {
                snprintf(g_ioBuffer,128,"%s/%s/%s",base,dir,iconSelected);
                emu.iconSelected=g_ioBuffer;
            }
        } else emu.iconSelected=emu.icon;
        if (*launch=='/')emu.launch=launch;
        else {
            snprintf(g_ioBuffer,128,"%s/%s/%s",base,dir,launch);
            emu.launch=g_ioBuffer;
        }
        if (rompath) {
            if (*rompath=='/')emu.path=rompath;
            else {
                snprintf(g_ioBuffer,128,"%s/%s/%s",base,dir,rompath);
                emu.path=g_ioBuffer;
            }
        }
        if (imgpath) {
            if (*imgpath=='/')emu.imagePath=imgpath;
            else {
                snprintf(g_ioBuffer,128,"%s/%s/%s",base,dir,imgpath);
                emu.imagePath=g_ioBuffer;
            }
        }
        removeEmu(list,emu.launch);
        printf("add %s run(%s), rom(%s), img(%s)\n",emu.label.c_str(),emu.launch.c_str(),emu.path.c_str(),emu.imagePath.c_str());
        int i=0;
        while (i<list->size()) {
            if (strcmp(list->at(i).label.c_str(),label)>0)break;
            i++;
        }
        if (i<list->size())list->insert(list->begin()+i,emu);
        else list->push_back(emu);
    }
}

// 0x122470
int loadEmuFolder(std::vector<EmuInfo>*list,const char*base) {
    return loadConfigFolder(list,base,"config.json",parseEmu);
}

// 0x1224b0
int parseThemeColor(const char*section,const char*text,SDL_Color*color) {
    if (strlen(text)>=7&&*text=='#') {
        char bytes[4]= {
            0
        };
        memcpy(bytes,text+1,2);
        color->r=strtoul(bytes,NULL,16);
        memcpy(bytes,text+3,2);
        color->g=strtoul(bytes,NULL,16);
        memcpy(bytes,text+5,2);
        color->b=strtoul(bytes,NULL,16);
        return 1;
    }
    return 0;
}

// 0x1225b4
void parseFont(const char*section,cJSON*root,std::string*font,std::string*ascii,int*size,SDL_Color*color) {
    cJSON*item,*number,*colorItem;
    item=cJSON_GetObjectItem(root,"font");
    if (item&&cJSON_IsString(item))*font=item->valuestring;
    item=cJSON_GetObjectItem(root,"fontascii");
    if (item&&cJSON_IsString(item))*ascii=item->valuestring;
    else *ascii=*font;
    number=cJSON_GetObjectItem(root,"size");
    if (number&&cJSON_IsNumber(number))*size=number->valueint;
    colorItem=cJSON_GetObjectItem(root,"color");
    if (colorItem&&cJSON_IsString(colorItem))parseThemeColor(section,colorItem->valuestring,color);
}

// 0x12276c
void parseListFont(const char*section,cJSON*root,std::string*font,std::string*ascii,int*size,SDL_Color*color,SDL_Color*selectedcolor,SDL_Color*highlightcolor) {
    cJSON*item,*number,*colorItem;
    item=cJSON_GetObjectItem(root,"font");
    if (item&&cJSON_IsString(item))*font=item->valuestring;
    item=cJSON_GetObjectItem(root,"fontascii");
    if (item&&cJSON_IsString(item))*ascii=item->valuestring;
    else *ascii=*font;
    number=cJSON_GetObjectItem(root,"size");
    if (number&&cJSON_IsNumber(number))*size=number->valueint;
    colorItem=cJSON_GetObjectItem(root,"color");
    if (colorItem&&cJSON_IsString(colorItem))parseThemeColor(section,colorItem->valuestring,color);
    colorItem=cJSON_GetObjectItem(root,"selectedcolor");
    if (colorItem&&cJSON_IsString(colorItem))parseThemeColor(section,colorItem->valuestring,selectedcolor);
    colorItem=cJSON_GetObjectItem(root,"highlightcolor");
    if (colorItem&&cJSON_IsString(colorItem))parseThemeColor(section,colorItem->valuestring,highlightcolor);
}

// 0x1229ec
void parseGridFont(const char*section,cJSON*root,std::string*font,std::string*ascii,int*grid1x4,int*grid3x4,int*iconxoff1x4,int*iconyoff1x4,int*txtyoff1x4,SDL_Color*color,SDL_Color*selectedcolor) {
    cJSON*item,*number,*colorItem;
    item=cJSON_GetObjectItem(root,"font");
    if (item&&cJSON_IsString(item))*font=item->valuestring;
    item=cJSON_GetObjectItem(root,"fontascii");
    if (item&&cJSON_IsString(item))*ascii=item->valuestring;
    else *ascii=*font;
    number=cJSON_GetObjectItem(root,"grid1x4");
    if (number&&cJSON_IsNumber(number))*grid1x4=number->valueint;
    number=cJSON_GetObjectItem(root,"grid3x4");
    if (number&&cJSON_IsNumber(number))*grid3x4=number->valueint;
    number=cJSON_GetObjectItem(root,"iconxoff1x4");
    if (number&&cJSON_IsNumber(number))*iconxoff1x4=number->valueint;
    number=cJSON_GetObjectItem(root,"iconyoff1x4");
    if (number&&cJSON_IsNumber(number))*iconyoff1x4=number->valueint;
    number=cJSON_GetObjectItem(root,"txtyoff1x4");
    if (number&&cJSON_IsNumber(number))*txtyoff1x4=number->valueint;
    colorItem=cJSON_GetObjectItem(root,"color");
    if (colorItem&&cJSON_IsString(colorItem))parseThemeColor(section,colorItem->valuestring,color);
    colorItem=cJSON_GetObjectItem(root,"selectedcolor");
    if (colorItem&&cJSON_IsString(colorItem))parseThemeColor(section,colorItem->valuestring,selectedcolor);
}

const SDL_Color g_configWhite= {
    240,240,240,0
}
,g_configGray= {
    104,104,104,0
}
,g_configDim= {
    170,170,170,0
}
,g_configYellow= {
    255,255,0,0
};
// 0x122d78
void parseTheme(void*data,const char*base,const char*dir,cJSON*root) {
    std::vector<Theme>*list=(std::vector<Theme>*)data;
    cJSON*item=NULL,*section=NULL;
    char*name=NULL;
    Theme theme;
    item=cJSON_GetObjectItem(root,"name");
    if (item) {
        std::string unused;
        name=cJSON_GetStringValue(item);
        theme.name=name;
        theme.root=base;
        theme.root+="/";
        theme.root+=dir;
        theme.root+="/";
        section=cJSON_GetObjectItem(root,"title");
        theme.titleSize=36;
        theme.titleColor.bytes=g_configWhite;
        if (section)parseFont("title",section,&theme.titleFontMulti,&theme.titleFontEnglish,&theme.titleSize,&theme.titleColor.bytes);
        section=cJSON_GetObjectItem(root,"path");
        theme.hintSize=24;
        theme.hintColor.bytes=g_configWhite;
        if (section)parseFont("path",section,&theme.hintFont,&unused,&theme.hintSize,&theme.hintColor.bytes);
        section=cJSON_GetObjectItem(root,"hint");
        theme.labelSize=40;
        theme.labelColor.bytes=g_configWhite;
        if (section)parseFont("hint",section,&theme.labelFontMulti,&theme.labelFontEnglish,&theme.labelSize,&theme.labelColor.bytes);
        section=cJSON_GetObjectItem(root,"currentpage");
        theme.listSize=40;
        theme.listColor.bytes=g_configWhite;
        if (section)parseFont("currentpage",section,&theme.listFont,&unused,&theme.listSize,&theme.listColor.bytes);
        section=cJSON_GetObjectItem(root,"total");
        theme.emptySize=40;
        theme.emptyColor.bytes=g_configDim;
        if (section)parseFont("total",section,&theme.emptyFont,&unused,&theme.emptySize,&theme.emptyColor.bytes);
        section=cJSON_GetObjectItem(root,"grid");
        theme.iconSize=25;
        theme.smallSize=18;
        theme.selectedColor.bytes=g_configGray;
        theme.textColor.bytes=g_configWhite;
        theme.field144=0;
        theme.field140=0;
        theme.field148=0;
        if (section)parseGridFont("grid",section,&theme.iconFontMulti,&theme.iconFontEnglish,&theme.iconSize,&theme.smallSize,&theme.field144,&theme.field140,&theme.field148,&theme.selectedColor.bytes,&theme.textColor.bytes);
        section=cJSON_GetObjectItem(root,"list");
        theme.mainSize=24;
        theme.imeColor.bytes=g_configWhite;
        theme.wifiColor.bytes=g_configWhite;
        theme.accentColor.bytes=g_configYellow;
        if (section)parseListFont("list",section,&theme.mainFontMulti,&theme.mainFontEnglish,&theme.mainSize,&theme.imeColor.bytes,&theme.wifiColor.bytes,&theme.accentColor.bytes);
        list->push_back(theme);
    }
}

// 0x12329c
int loadThemeFolder(std::vector<Theme>*list,const char*base) {
    return loadConfigFolder(list,base,"config.json",parseTheme);
}

// 0x1232dc
Theme*findTheme(std::vector<Theme>*list,const char*name) {
    if (name) {
        for (int i=0;i<list->size();i++) {
            if (strcmp((*list)[i].root.c_str(),name)==0)return &(*list)[i];
        }
    }
    return NULL;
}

// 0x1233b0
void replaceAll(std::string&text,const std::string&from,const std::string&to) {
    unsigned pos=0;
    while (true) {
        pos=text.find(from,pos);
        if (pos==std::string::npos)break;
        text.replace(pos,from.length(),to.c_str());
        pos+=to.length();
    }
}

int getSerial(char*);
// 0x123444
bool loadTestResults(int*results,char*serial) {
    FILE*f=fopen("/mnt/SDCARD/test_result.json","r");
    char key[32]= {
        0
    };
    bool ok=true;
    if (f) {
        char*content=NULL;
        if (readWholeFile("/mnt/SDCARD/test_result.json",f,&content)>=0) {
            cJSON*root=cJSON_Parse(content);
            if (root) {
                for (int i=0;i<5;i++) {
                    snprintf(key,32,"testitem%d",i);
                    cJSON*item=cJSON_GetObjectItem(root,key);
                    if (item&&item->type==8) {
                        if (results)results[i]=item->valueint;
                    } else ok=false;
                }
                cJSON*item=cJSON_GetObjectItem(root,"serial");
                if (item&&item->type==16) {
                    strncpy(serial,item->valuestring,15);
                    serial[15]=0;
                } else *serial=0;
                cJSON_Delete(root);
            } else ok=false;
            free(content);
        } else ok=false;
        fclose(f);
    } else ok=false;
    return ok;
}

// 0x123658
int saveTestResults(int*results) {
    if (results) {
        cJSON*root=cJSON_CreateObject();
        char key[16]= {
            0
        };
        if (root) {
            for (int i=0;i<5;i++) {
                snprintf(key,16,"testitem%d",i);
                cJSON_AddNumberToObject(root,key,results[i]);
            }
            getSerial(key);
            cJSON_AddStringToObject(root,"serial",key);
            char*text=cJSON_PrintUnformatted(root);
            FILE*f=fopen("/mnt/SDCARD/test_result.json","w");
            if (f) {
                fprintf(f,"%s\n",text);
                fclose(f);
            }
            free(text);
            cJSON_Delete(root);
            sync();
            return 0;
        }
    }
    return -1;
}

extern unsigned char g_deviceId[5];
// 0x1237a8
int getSerial(char*serial) {
    unsigned value=(unsigned)g_deviceId[3]<<24|(unsigned)g_deviceId[2]<<16|(unsigned)g_deviceId[1]<<8|(unsigned)g_deviceId[0];
    value+=0x1583247;
    return snprintf(serial,16,"%02X%08X",(unsigned)g_deviceId[4],value);
}

// 0x123858
bool languageFontExists() {
    return access("/mnt/SDCARD/wqy-microhei.ttc",0)==0||access("./wqy-microhei.ttc",0)==0;
}

// 0x1238ac
unsigned long milliseconds() {
    timeval tv;
    gettimeofday(&tv,NULL);
    unsigned long value=tv.tv_sec*1000UL+(unsigned)tv.tv_usec/1000;
    return value;
}

struct SDL_PixelFormat {
    void*palette;
    unsigned char bitsPerPixel,bytesPerPixel,rLoss,gLoss,bLoss,aLoss,rShift,gShift,bShift,aShift;
    unsigned rMask,gMask,bMask,aMask;
};
// 0x123908
void printSurfaceFormat(const char*label,SDL_Surface*surface) {
    printf("%s flags %x bpp %d loss(%d, %d, %d, %d) shift(%d, %d, %d, %d) mask(%x, %x, %x, %x)\n",label,surface->flags,surface->format->bitsPerPixel,surface->format->rLoss,surface->format->gLoss,surface->format->bLoss,surface->format->aLoss,surface->format->rShift,surface->format->gShift,surface->format->bShift,surface->format->aShift,surface->format->rMask,surface->format->gMask,surface->format->bMask,surface->format->aMask);
}

extern "C" SDL_Surface*IMG_Load(const char*);
extern "C" SDL_Surface*SDL_ConvertSurface(SDL_Surface*,SDL_PixelFormat*,unsigned);
extern "C" void SDL_FreeSurface(SDL_Surface*);
// 0x123a38
SDL_Surface*loadImage(const char*path,SDL_Surface*format,bool debug) {
    SDL_Surface*src=IMG_Load(path);
    if (src) {
        SDL_Surface*converted=SDL_ConvertSurface(src,src->format,0);
        if (converted) {
            if (debug) {
                printSurfaceFormat("   src format:",src);
                printSurfaceFormat("target format:",converted);
            }
            SDL_FreeSurface(src);
            return converted;
        }
        return src;
    }
    return NULL;
}

// 0x123af0
SDL_Surface*loadSkinImage(const char*base,const char*fallback,const char*name,SDL_Surface*format,bool debug) {
    std::string path;
    if (*name!='/') {
        path=base;
        path+=name;
        SDL_Surface*image=loadImage(path.c_str(),format,debug);
        if (image) {
            printf("%s/%s open ok\n",base,name);
            return image;
        } else {
            path=fallback;
            path+=name;
            printf("%s/%s open fail, use %s/%s\n",base,name,fallback,name);
            return loadImage(path.c_str(),format,debug);
        }
    }
    path=name;
    SDL_Surface*image=loadImage(path.c_str(),format,debug);
    if (!image)printf("%s open ok\n",path.c_str());
    return image;
}

// 0x123c8c
int readTextFile(const char*path,char*buf,int length) {
    int fd=open(path,O_RDONLY);
    if (fd<0) {
        *buf=0;
        return -1;
    }
    int count=read(fd,buf,length);
    buf[count>0?count:0]=0;
    close(fd);
    return count;
}

// 0x123d28
int findProcess(const char*name,int debug) {
    int pid=0;
    size_t length=strlen(name);
    DIR*d;
    if ((d=opendir("/proc"))==NULL) {
        puts("can not open /proc");
        return -1;
    }
    while (true) {
        dirent*entry;
        if ((entry=readdir(d))==NULL)goto process_done;
        char*id=entry->d_name;
        if (*id<'0'||*id>'9')continue;
        char buf[256];
        sprintf(buf,"/proc/%s/cmdline",id);
        readTextFile(buf,buf,255);
        if (strlen(buf)<length)continue;
        char*found=strstr(buf,name);
        if (found&&debug)printf("[check: %s] p=%s in %s, pnlen=%d, cmp %d, p[%d]=%d\n",name,found,buf,length,strncmp(found,name,length),length,found[length]);
        if (found&&strncmp(found,name,length)==0&&(found[length]==0||found[length]==' ')) {
            pid=atoi(id);
            closedir(d);
            if (debug)printf("[check: %s] pid %d\n",name,pid);
            return pid;
        }
    }
process_done:; // Shared failure exit preserves the original loop layout.
    closedir(d);
    if (debug)printf("[check: %s] fail\n",name);
    return -1;
}
