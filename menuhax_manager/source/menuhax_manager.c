#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <3ds.h>

#include <lodepng.h>

#include "archive.h"

#include "default_imagedisplay_png.h"

u8 *filebuffer;
u32 filebuffer_maxsize = 0x400000;

char regionids_table[7][4] = {//http://3dbrew.org/wiki/Nandrw/sys/SecureInfo_A
"JPN",
"USA",
"EUR",
"JPN", //"AUS"
"CHN",
"KOR",
"TWN"
};

u32 NVer_tidlow_regionarray[7] = {
0x00016202, //JPN
0x00016302, //USA
0x00016102, //EUR
0x00016202, //"AUS"
0x00016402, //CHN
0x00016502, //KOR
0x00016602, //TWN
};

u32 CVer_tidlow_regionarray[7] = {
0x00017202, //JPN
0x00017302, //USA
0x00017102, //EUR
0x00017202, //"AUS"
0x00017402, //CHN
0x00017502, //KOR
0x00017602 //TWN
};

void display_menu(char **menu_entries, int total_entries, int *menuindex, char *headerstr)
{
	int i;
	u32 redraw = 1;
	u32 kDown = 0;

	while(1)
	{
		gspWaitForVBlank();
		hidScanInput();

		kDown = hidKeysDown();

		if(redraw)
		{
			redraw = 0;

			consoleClear();
			printf("%s.\n\n", headerstr);

			for(i=0; i<total_entries; i++)
			{
				if(*menuindex==i)
				{
					printf("-> ");
				}
				else
				{
					printf("   ");
				}

				printf("%s\n", menu_entries[i]);
			}
		}

		if(kDown & KEY_B)
		{
			*menuindex = -1;
			return;
		}

		if(kDown & (KEY_DDOWN | KEY_CPAD_DOWN))
		{
			(*menuindex)++;
			if(*menuindex>=total_entries)*menuindex = 0;
			redraw = 1;

			continue;
		}
		else if(kDown & (KEY_DUP | KEY_CPAD_UP))
		{
			(*menuindex)--;
			if(*menuindex<0)*menuindex = total_entries-1;
			redraw = 1;

			continue;
		}

		if(kDown & KEY_A)
		{
			break;
		}
	}
}

void displaymessage_waitbutton()
{
	printf("\nPress the A button to continue.\n");
	while(1)
	{
		gspWaitForVBlank();
		hidScanInput();
		if(hidKeysDown() & KEY_A)break;
	}
}

Result enablethemecache(u32 type)
{	
	Result ret=0;
	u32 filesize = 0;

	printf("Reading SaveData.dat...\n");

	ret = archive_getfilesize(HomeMenu_Extdata, "/SaveData.dat", &filesize);
	if(ret!=0)
	{
		printf("Failed to get filesize for extdata SaveData.dat: 0x%08x\n", (unsigned int)ret);
		return ret;
	}

	if(filesize > filebuffer_maxsize)
	{
		printf("Extdata SaveData.dat filesize is too large: 0x%08x\n", (unsigned int)filesize);
		return ret;
	}

	ret = archive_readfile(HomeMenu_Extdata, "/SaveData.dat", filebuffer, filesize);
	if(ret!=0)
	{
		printf("Failed to read file: 0x%08x\n", (unsigned int)ret);
		return ret;
	}

	if(ret==0)
	{
		if(filebuffer[0x141b]==0 && filebuffer[0x13b8]!=0 && filebuffer[0x13bc]==0 && filebuffer[0x13bd]==type)
		{
			ret = 0;
			printf("SaveData.dat is already set for using the theme cache with a regular theme.\n");
			return ret;
		}
	}

	if(ret==0)
	{
		filebuffer[0x141b]=0;//Disable theme shuffle.
		memset(&filebuffer[0x13b8], 0, 8);//Clear the regular-theme structure.
		filebuffer[0x13bd]=type;//theme-type
		filebuffer[0x13b8] = 0xff;//theme-index

		printf("Writing updated SaveData.dat...\n");

		ret = archive_writefile(HomeMenu_Extdata, "/SaveData.dat", filebuffer, filesize);
		if(ret!=0)
		{
			printf("Failed to write file: 0x%08x\n", (unsigned int)ret);
		}
	}

	return ret;
}

Result menu_enablethemecache_persistent()
{
	return enablethemecache(3);
}

Result disablethemecache()
{
	Result ret=0;
	u32 filesize = 0;

	printf("Reading SaveData.dat...\n");

	ret = archive_getfilesize(HomeMenu_Extdata, "/SaveData.dat", &filesize);
	if(ret!=0)
	{
		printf("Failed to get filesize for extdata SaveData.dat: 0x%08x\n", (unsigned int)ret);
		return ret;
	}

	if(filesize > filebuffer_maxsize)
	{
		printf("Extdata SaveData.dat filesize is too large: 0x%08x\n", (unsigned int)filesize);
		return ret;
	}

	ret = archive_readfile(HomeMenu_Extdata, "/SaveData.dat", filebuffer, filesize);
	if(ret!=0)
	{
		printf("Failed to read file: 0x%08x\n", (unsigned int)ret);
		return ret;
	}

	if(ret==0)
	{
		filebuffer[0x141b]=0;//Disable theme shuffle.
		memset(&filebuffer[0x13b8], 0, 8);//Clear the regular-theme structure.

		printf("Writing updated SaveData.dat...\n");

		ret = archive_writefile(HomeMenu_Extdata, "/SaveData.dat", filebuffer, filesize);
		if(ret!=0)
		{
			printf("Failed to write file: 0x%08x\n", (unsigned int)ret);
		}
	}

	return ret;
}

int sd2themecache(char *body_filepath, char *bgm_filepath, u32 install_type)
{
	Result ret=0;
	u32 body_size=0, bgm_size=0;
	u32 thememanage[0x20>>2];

	memset(thememanage, 0, sizeof(thememanage));

	ret = archive_getfilesize(SDArchive, body_filepath, &body_size);
	if(ret!=0)
	{
		printf("Failed to get the filesize of the body-filepath: %s\n", body_filepath);
		if(install_type==0)
		{
			printf("Verify that you have a menuhax build for your system on SD card: make sure that the release archive you're using actually includes builds for your system-version.\n");
			printf("Also verify that the following directory containing .lz files actually exists on your SD card: '/3ds/menuhax_manager/themepayload/'.\n");
		}
		return ret;
	}
	else
	{
		printf("Using body-filepath: %s\n", body_filepath);
	}

	if(body_size==0)
	{
		printf("Error: the theme body-data file is empty(filesize is 0-bytes).\n");
		return -1;
	}

	if(bgm_filepath)
	{
		ret = archive_getfilesize(SDArchive, bgm_filepath, &bgm_size);
		if(ret!=0)
		{
			ret = archive_getfilesize(SDArchive, bgm_filepath, &bgm_size);
			printf("Skipping BGM copying since  failed for it.\n");

			bgm_size = 0;
		}
		else
		{
			printf("Using bgm-filepath: %s\n", bgm_filepath);
		}
	}

	printf("Generating a ThemeManage.bin + writing it to extdata...\n");

	memset(thememanage, 0, 0x20);
	thememanage[0x0>>2] = 1;
	thememanage[0x8>>2] = body_size;
	thememanage[0xC>>2] = bgm_size;
	thememanage[0x10>>2] = 0xff;
	thememanage[0x14>>2] = 1;
	thememanage[0x18>>2] = 0xff;
	thememanage[0x1c>>2] = 0x200;

	memset(filebuffer, 0, 0x800);
	memcpy(filebuffer, thememanage, 0x20);
	ret = archive_writefile(Theme_Extdata, install_type==0 ? "/ThemeManage.bin" : "/yhemeManage.bin", filebuffer, 0x800);

	if(ret!=0)
	{
		printf("Failed to write ThemeManage.bin to extdata, aborting.\n");
		return ret;
	}

	ret = archive_copyfile(SDArchive, Theme_Extdata, body_filepath, install_type==0 ? "/BodyCache.bin" : "/yodyCache.bin", filebuffer, thememanage[0x8>>2], 0x150000, "body-data");

	if(ret==0)
	{
		printf("Successfully finished copying body-data.\n");
	}
	else
	{
		return ret;
	}

	if(bgm_filepath && bgm_size)
	{
		ret = archive_copyfile(SDArchive, Theme_Extdata, bgm_filepath, "/BgmCache.bin", filebuffer, thememanage[0xC>>2], 0x337000, "bgm-data");

		if(ret==0)
		{
			printf("Successfully finished copying bgm-data.\n");
		}
		else
		{
			return ret;
		}
	}

	return 0;
}

Result delete_menuhax()
{
	Result ret=0;
	u32 i;
	char str[256];

	gspWaitForVBlank();
	hidScanInput();

	if(hidKeysHeld() & KEY_X)
	{
		printf("Skipping menuhax deletion since the X button is pressed.\n");
	}
	else
	{
		printf("Deleting menuhax since the X button isn't pressed.\n");

		printf("Disabling theme-usage via SaveData.dat...\n");
		ret = disablethemecache();
		if(ret!=0)return ret;

		memset(filebuffer, 0, filebuffer_maxsize);

		printf("Clearing the theme-cache extdata now...\n");

		printf("Clearing the regular ThemeManage...\n");
		ret = archive_writefile(Theme_Extdata, "/ThemeManage.bin", filebuffer, 0x800);
		if(ret!=0)
		{
			printf("Failed to clear the regular ThemeManage: 0x%08x.\n", (unsigned int)ret);
			return ret;
		}

		printf("Clearing the regular BodyCache...\n");
		ret = archive_writefile(Theme_Extdata, "/BodyCache.bin", filebuffer, 0x150000);
		if(ret!=0)
		{
			printf("Failed to clear the regular BodyCache: 0x%08x.\n", (unsigned int)ret);
			return ret;
		}

		printf("Clearing the regular BgmCache...\n");
		ret = archive_writefile(Theme_Extdata, "/BgmCache.bin", filebuffer, 0x337000);
		if(ret!=0)
		{
			printf("Failed to clear the regular BgmCache: 0x%08x.\n", (unsigned int)ret);
			return ret;
		}

		printf("The menuhax itself has been deleted successfully.\n");
	}

	printf("Deleting the additional menuhax files under theme-cache extdata now. This can only work if Home Menu theme-settings menu was entered at least once with menuhax >=v2.0 installed. Note that failing to delete theme-shuffle data is normal.\n");

	printf("Deleting the menuhax regular ThemeManage...\n");
	ret = archive_deletefile(Theme_Extdata, "/yhemeManage.bin");
	if(ret!=0)
	{
		printf("Failed to delete the menuhax regular ThemeManage: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	printf("Deleting the menuhax regular BodyCache...\n");
	ret = archive_deletefile(Theme_Extdata, "/yodyCache.bin");
	if(ret!=0)
	{
		printf("Failed to delete the menuhax regular BodyCache: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	printf("Deleting menuhax shuffle BodyCache...\n");
	ret = archive_deletefile(Theme_Extdata, "/yodyCache_rd.bin");
	if(ret!=0)
	{
		printf("Failed to delete the menuhax shuffle BodyCache: 0x%08x.\n", (unsigned int)ret);
	}

	if(ret==0)
	{
		for(i=0; i<10; i++)
		{
			memset(str, 0, sizeof(str));
			snprintf(str, sizeof(str)-1, "/yhemeManage_%02d.bin", (unsigned int)i);

			printf("Deleting menuhax shuffle ThemeManage_%02d...\n", (unsigned int)i);
			ret = archive_deletefile(Theme_Extdata, str);
			if(ret!=0)
			{
				printf("Failed to delete menuhax shuffle ThemeManage_%02d: 0x%08x.\n", (unsigned int)i, (unsigned int)ret);
				break;
			}
		}
	}

	if(ret==0)
	{
		for(i=0; i<10; i++)
		{
			memset(str, 0, sizeof(str));
			snprintf(str, sizeof(str)-1, "/yodyCache_%02d.bin", (unsigned int)i);

			printf("Deleting menuhax shuffle BodyCache_%02d...\n", (unsigned int)i);
			ret = archive_deletefile(Theme_Extdata, str);
			if(ret!=0)
			{
				printf("Failed to delete menuhax shuffle BodyCache_%02d: 0x%08x.\n", (unsigned int)i, (unsigned int)ret);
				break;
			}
		}
	}

	return 0;
}

Result setup_builtin_theme()
{
	Result ret=0;
	int menuindex = 0;
	u32 filesize=0;

	Handle filehandle = 0;

	u32 file_lowpath_data[0xc>>2];

	FS_archive archive = { ARCH_ROMFS, { PATH_EMPTY, 1, (u8*)"" }, 0, 0 };
	FS_path fileLowPath;

	char *menu_entries[] = {
	"Red",
	"Blue",
	"Yellow",
	"Pink",
	"Black"};

	char str[64];
	char str2[64];

	memset(file_lowpath_data, 0, sizeof(file_lowpath_data));

	fileLowPath.type = PATH_BINARY;
	fileLowPath.size = 0xc;
	fileLowPath.data = (u8*)file_lowpath_data;

	display_menu(menu_entries, 5, &menuindex, "Select a built-in Home Menu theme for installation with the below menu. You can press the B button to exit. Note that this implementation can only work when this app is running from *hax payloads >=v2.0(https://smealum.github.io/3ds/). If you hold down the X button while selecting a theme via the A button, the theme-data will also be written to 'sdmc:/3ds/menuhax_manager/'.");

	if(menuindex==-1)return 0;

	ret = FSUSER_OpenFileDirectly(NULL, &filehandle, archive, fileLowPath, FS_OPEN_READ, 0x0);
	if(ret!=0)
	{
		printf("Failed to open the RomFS image for the current process: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	ret = romfsInitFromFile(filehandle, 0x0);
	if(ret!=0)
	{
		printf("Failed to mount the RomFS image for the current process: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	printf("Enabling persistent themecache...\n");
	ret = menu_enablethemecache_persistent();
	if(ret==0)
	{
		memset(str, 0, sizeof(str));
		snprintf(str, sizeof(str)-1, "romfs:/theme/%s_LZ.bin", menu_entries[menuindex]);

		printf("Using the following theme: %s\n", str);

		gspWaitForVBlank();
		hidScanInput();

		if(hidKeysHeld() & KEY_X)
		{
			memset(str2, 0, sizeof(str2));
			snprintf(str2, sizeof(str2)-1, "sdmc:/3ds/menuhax_manager/%s_LZ.bin", menu_entries[menuindex]);

			printf("Since the X button is being pressed, copying the built-in theme to '%s'...\n", str2);

			ret = archive_getfilesize(SDArchive, str, &filesize);
			if(ret!=0)
			{
				printf("Failed to get the filesize for the theme-data: 0x%08x.\n", (unsigned int)ret);
			}
			else
			{
				ret = archive_copyfile(SDArchive, SDArchive, str, str2, filebuffer, filesize, 0x150000, "body-data");
				if(ret!=0)
				{
					printf("Copy failed: 0x%08x.\n", (unsigned int)ret);
				}
			}
		}
		else
		{
			printf("Skipping theme-dumping since the X button isn't pressed.\n");
		}

		ret = sd2themecache(str, NULL, 1);
	}

	romfsExit();

	return ret;
}

Result http_getactual_payloadurl(char *requrl, char *outurl, u32 outurl_maxsize)
{
	Result ret=0;
	httpcContext context;

	ret = httpcOpenContext(&context, requrl, 1);
	if(ret!=0)return ret;

	ret = httpcAddRequestHeaderField(&context, "User-Agent", "menuhax_manager/"VERSION);
	if(ret!=0)
	{
		httpcCloseContext(&context);
		return ret;
	}

	ret = httpcBeginRequest(&context);
	if(ret!=0)
	{
		httpcCloseContext(&context);
		return ret;
	}

	ret = httpcGetResponseHeader(&context, "Location", outurl, outurl_maxsize);

	httpcCloseContext(&context);

	return 0;
}

Result http_download_payload(char *url, u32 *payloadsize)
{
	Result ret=0;
	u32 statuscode=0;
	u32 contentsize=0;
	httpcContext context;

	ret = httpcOpenContext(&context, url, 1);
	if(ret!=0)return ret;

	ret = httpcAddRequestHeaderField(&context, "User-Agent", "menuhax_manager/"VERSION);
	if(ret!=0)
	{
		httpcCloseContext(&context);
		return ret;
	}

	ret = httpcBeginRequest(&context);
	if(ret!=0)
	{
		httpcCloseContext(&context);
		return ret;
	}

	ret = httpcGetResponseStatusCode(&context, &statuscode, 0);
	if(ret!=0)
	{
		httpcCloseContext(&context);
		return ret;
	}

	if(statuscode!=200)
	{
		printf("Error: server returned HTTP statuscode %u.\n", (unsigned int)statuscode);
		httpcCloseContext(&context);
		return -2;
	}

	ret=httpcGetDownloadSizeState(&context, NULL, &contentsize);
	if(ret!=0)
	{
		httpcCloseContext(&context);
		return ret;
	}

	if(contentsize==0 || contentsize>filebuffer_maxsize)
	{
		printf("Invalid HTTP content-size: 0x%08x.\n", (unsigned int)contentsize);
		ret = -3;
		httpcCloseContext(&context);
		return ret;
	}

	ret = httpcDownloadData(&context, filebuffer, contentsize, NULL);
	if(ret!=0)
	{
		httpcCloseContext(&context);
		return ret;
	}

	httpcCloseContext(&context);

	*payloadsize = contentsize;

	return 0;
}

Result read_versionbin(FS_archive archive, FS_path fileLowPath, u8 *versionbin)
{
	Result ret = 0;
	Handle filehandle = 0;

	ret = FSUSER_OpenFileDirectly(NULL, &filehandle, archive, fileLowPath, FS_OPEN_READ, 0x0);
	if(ret!=0)
	{
		printf("Failed to open the RomFS image for *Ver: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	ret = romfsInitFromFile(filehandle, 0x0);
	if(ret!=0)
	{
		printf("Failed to mount the RomFS image for *Ver: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	ret = archive_readfile(SDArchive, "romfs:/version.bin", versionbin, 0x8);
	romfsExit();

	if(ret!=0)
	{
		printf("Failed to read the *Ver version.bin: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	return 0;
}

Result install_menuhax(char *ropbin_filepath)
{
	Result ret = 0;
	u8 region=0;
	u8 new3dsflag = 0;
	u64 menu_programid = 0;
	TitleList menu_title_entry;
	u32 payloadinfo[4];
	char menuhax_basefn[256];
	char body_filepath[256];
	u32 archive_lowpath_data[0x10>>2];//+0 = programID-low, +4 = programID-high, +8 = u8 mediatype.
	u32 file_lowpath_data[0x14>>2];

	FS_archive archive;
	FS_path fileLowPath;

	u8 nver_versionbin[0x8];
	u8 cver_versionbin[0x8];

	u32 payloadsize = 0;

	char payloadurl[0x80];

	memset(menuhax_basefn, 0, sizeof(menuhax_basefn));
	memset(body_filepath, 0, sizeof(body_filepath));

	memset(payloadinfo, 0, sizeof(payloadinfo));

	memset(archive_lowpath_data, 0, sizeof(file_lowpath_data));
	memset(file_lowpath_data, 0, sizeof(file_lowpath_data));

	memset(nver_versionbin, 0, sizeof(nver_versionbin));
	memset(cver_versionbin, 0, sizeof(cver_versionbin));

	memset(payloadurl, 0, sizeof(payloadurl));

	archive.id = 0x2345678a;
	archive.lowPath.type = PATH_BINARY;
	archive.lowPath.size = 0x10;
	archive.lowPath.data = (u8*)archive_lowpath_data;

	fileLowPath.type = PATH_BINARY;
	fileLowPath.size = 0x14;
	fileLowPath.data = (u8*)file_lowpath_data;

	archive_lowpath_data[1] = 0x000400DB;

	printf("Getting system-version/system-info etc...\n");

	ret = initCfgu();
	if(ret!=0)
	{
		printf("Failed to init cfgu: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}
	ret = CFGU_SecureInfoGetRegion(&region);
	if(ret!=0)
	{
		printf("Failed to get region from cfgu: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}
	if(region>=7)
	{
		printf("Region value from cfgu is invalid: 0x%02x.\n", (unsigned int)region);
		ret = -9;
		return ret;
	}
	exitCfgu();

	APT_CheckNew3DS(NULL, &new3dsflag);

	aptOpenSession();
	ret = APT_GetAppletInfo(NULL, APPID_HOMEMENU, &menu_programid, NULL, NULL, NULL, NULL);
	aptCloseSession();

	if(ret!=0)
	{
		printf("Failed to get the Home Menu programID: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	printf("Using Home Menu programID: 0x%016llx.\n", menu_programid);

	ret = AM_ListTitles(0, 1, &menu_programid, &menu_title_entry);
	if(ret!=0)
	{
		printf("Failed to get the Home Menu title-version: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	snprintf(menuhax_basefn, sizeof(menuhax_basefn)-1, "menuhax_%s%u_%s", regionids_table[region], menu_title_entry.titleVersion, new3dsflag?"new3ds":"old3ds");
	snprintf(body_filepath, sizeof(body_filepath)-1, "sdmc:/3ds/menuhax_manager/themepayload/%s.lz", menuhax_basefn);
	snprintf(ropbin_filepath, 255, "sdmc:/ropbinpayload_%s.bin", menuhax_basefn);

	archive_lowpath_data[0] = NVer_tidlow_regionarray[region];
	ret = read_versionbin(archive, fileLowPath, nver_versionbin);

	if(ret!=0)
	{
		printf("Failed to read the NVer version.bin: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	archive_lowpath_data[0] = CVer_tidlow_regionarray[region];
	ret = read_versionbin(archive, fileLowPath, cver_versionbin);

	if(ret!=0)
	{
		printf("Failed to read the CVer version.bin: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}

	snprintf(payloadurl, sizeof(payloadurl)-1, "http://smea.mtheall.com/get_ropbin_payload.php?version=%s-%d-%d-%d-%d-%s", new3dsflag?"NEW":"OLD", cver_versionbin[2], cver_versionbin[1], cver_versionbin[0], nver_versionbin[2], regionids_table[region]);

	printf("Detected system-version: %s %d.%d.%d-%d %s\n", new3dsflag?"New3DS":"Old3DS", cver_versionbin[2], cver_versionbin[1], cver_versionbin[0], nver_versionbin[2], regionids_table[region]);

	if(((cver_versionbin[2]<<8) | cver_versionbin[1]) >= 0x0a02)
	{
		printf("You are attempting to install Home Menu haxx on system-version >=v10.2. This build does not currently include any menuhax which supports this system-version, since v10.2 fixed themehax.\n");
		return -7;
	}

	memset(filebuffer, 0, filebuffer_maxsize);

	gspWaitForVBlank();
	hidScanInput();
	if(hidKeysHeld() & KEY_X)
	{
		printf("The X button is being pressed, skipping ropbin payload setup. If this was not intended, re-run the install again after this.\n");
	}
	else
	{
		printf("The X button isn't being pressed, setting up ropbin payload...\n");

		printf("Checking for the input payload on SD...\n");
		ret = archive_getfilesize(SDArchive, "sdmc:/menuhaxmanager_input_payload.bin", &payloadsize);
		if(ret==0)
		{
			if(payloadsize==0 || payloadsize>filebuffer_maxsize)
			{
				printf("Invalid SD payload size: 0x%08x.\n", (unsigned int)payloadsize);
				ret = -3;
			}
		}
		if(ret==0)ret = archive_readfile(SDArchive, "sdmc:/menuhaxmanager_input_payload.bin", filebuffer, payloadsize);

		if(ret==0)
		{
			printf("The input payload for this installer already exists on SD, that will be used instead of downloading the payload via HTTP.\n");
		}
		else
		{
			ret = httpcInit();
			if(ret!=0)
			{
				printf("Failed to initialize HTTPC: 0x%08x.\n", (unsigned int)ret);
				if(ret==0xd8e06406)
				{
					printf("The HTTPC service is inaccessible. With the hblauncher-payload this may happen if the process this app is running under doesn't have access to that service. Please try rebooting the system, boot hblauncher-payload, then directly launch the app.\n");
				}

				return ret;
			}

			printf("Requesting the actual payload URL with HTTP...\n");
			ret = http_getactual_payloadurl(payloadurl, payloadurl, sizeof(payloadurl));
			if(ret!=0)
			{
				printf("Failed to request the actual payload URL: 0x%08x.\n", (unsigned int)ret);
				printf("If the server isn't down, and the HTTP request was actually done, this may mean your system-version or region isn't supported by the hblauncher-payload currently.\n");
				httpcExit();
				return ret;
			}

			printf("Downloading the actual payload with HTTP...\n");
			ret = http_download_payload(payloadurl, &payloadsize);
			httpcExit();
			if(ret!=0)
			{
				printf("Failed to download the actual payload with HTTP: 0x%08x.\n", (unsigned int)ret);
				printf("If the server isn't down, and the HTTP request was actually done, this may mean your system-version or region isn't supported by the hblauncher-payload currently.\n");
				return ret;
			}
		}

		printf("Writing the menuropbin to SD, to the following path: %s.\n", ropbin_filepath);
		unlink("sdmc:/menuhax_ropbinpayload.bin");//Delete the ropbin with the filepath used by the <=v1.2 menuhax.
		unlink(ropbin_filepath);
		ret = archive_writefile(SDArchive, ropbin_filepath, filebuffer, 0x10000);
		if(ret!=0)
		{
			printf("Failed to write the menurop to the SD file: 0x%08x.\n", (unsigned int)ret);
			return ret;
		}

		memset(filebuffer, 0, filebuffer_maxsize);
	}

	printf("Enabling persistent themecache...\n");
	ret = menu_enablethemecache_persistent();
	if(ret!=0)return ret;

	printf("Installing to the SD theme-cache...\n");
	ret = sd2themecache(body_filepath, "sdmc:/3ds/menuhax_manager/bgm_bundledmenuhax.bcstm", 0);
	if(ret!=0)return ret;

	return 0;
}

void print_padbuttons(u32 val)
{
	u32 i;

	if(val==0)
	{
		printf("<no-buttons>");
		return;
	}

	for(i=0; i<12; i++)
	{
		if((val & (1<<i))==0)continue;

		switch(1<<i)
		{
			case KEY_A:
				printf("A ");
			break;

			case KEY_B:
				printf("B ");
			break;

			case KEY_SELECT:
				printf("SELECT ");
			break;

			case KEY_START:
				printf("START ");
			break;

			case KEY_DRIGHT:
				printf("D-PAD RIGHT ");
			break;

			case KEY_DLEFT:
				printf("D-PAD LEFT ");
			break;

			case KEY_DUP:
				printf("D-PAD UP ");
			break;

			case KEY_DDOWN:
				printf("D-PAD DOWN ");
			break;

			case KEY_R:
				printf("R ");
			break;

			case KEY_L:
				printf("L ");
			break;

			case KEY_X:
				printf("X ");
			break;

			case KEY_Y:
				printf("Y ");
			break;
		}
	}
}

Result setup_sdcfg()
{
	Result ret=0;
	u32 kDown, padval=0;
	int menuindex = 0;

	u32 sdcfg[0x10>>2];//Last u32 is reserved atm.

	char *menu_entries[] = {
	"Type1: Only trigger the haxx when the PAD state matches the specified value(specified button(s) must be pressed).",
	"Type2: Only trigger the haxx when the PAD state doesn't match the specified value.",
	"Type0: Default PAD config is used.",
	"Delete the config file(same end result on the menuhax as type0 basically)."};

	printf("Configuring the padcfg file on SD...\n");

	memset(sdcfg, 0, sizeof(sdcfg));

	ret = archive_readfile(SDArchive, "sdmc:/menuhax_padcfg.bin", (u8*)sdcfg, sizeof(sdcfg));
	if(ret==0)
	{
		printf("The cfg file already exists on SD.\n");
		printf("Current cfg:\n");
		printf("Type 0x%x: ", (unsigned int)sdcfg[0]);

		if(sdcfg[0]==0x1)
		{
			printf("Only trigger the haxx when the PAD state matches the specified value(specified button(s) must be pressed).\n");
			printf("Currently selected PAD value: 0x%x ", (unsigned int)sdcfg[1]);
			print_padbuttons(sdcfg[1]);
			printf("\n");
		}
		else if(sdcfg[0]==0x2)
		{
			printf("Only trigger the haxx when the PAD state doesn't match the specified value.\n");
			printf("Currently selected PAD value: 0x%x ", (unsigned int)sdcfg[2]);
			print_padbuttons(sdcfg[2]);
			printf("\n");
		}
		else
		{
			printf("None, the default PAD trigger is used.\n");
		}
	}
	else
	{
		printf("The cfg file currently doesn't exist on SD.\n");
	}

	memset(sdcfg, 0, sizeof(sdcfg));

	displaymessage_waitbutton();

	display_menu(menu_entries, 4, &menuindex, "Select a type with the below menu. You can press B to exit without changing anything.");

	if(menuindex==-1)return 0;

	switch(menuindex)
	{
		case 0:
			sdcfg[0] = 0x1;
		break;

		case 1:
			sdcfg[0] = 0x2;
		break;

		case 2:
			sdcfg[0] = 0x0;
		break;

		case 3:
			unlink("sdmc:/menuhax_padcfg.bin");
		return 0;
	}

	if(sdcfg[0])
	{
		printf("Press the button(s) you want to select for the PAD state value as described above(no New3DS-only buttons). If you want to select <no-buttons>, don't press any buttons. Then, while the buttons are being pressed, if any, touch the bottom-screen.\n");

		while(1)
		{
			gspWaitForVBlank();
			hidScanInput();
			kDown = hidKeysHeld();

			if(kDown & KEY_TOUCH)
			{
				 padval = kDown & 0xfff;
				break;
			}
		}

		printf("Selected PAD value: 0x%x ", (unsigned int)padval);
		print_padbuttons(padval);
		sdcfg[sdcfg[0]] = padval;
		printf("\n");
	}

	ret = archive_writefile(SDArchive, "sdmc:/menuhax_padcfg.bin", (u8*)sdcfg, sizeof(sdcfg));
	if(ret!=0)printf("Failed to write the cfg file: 0x%x.\n", (unsigned int)ret);
	if(ret==0)printf("Config file successfully written.\n");

	return ret;
}

Result setup_imagedisplay()
{
	Result ret=0;
	int menuindex = 0;
	unsigned w = 0, h = 0, x, y, pos0, pos1;
	size_t pngsize = 0;
	u32 imgdisp_exists = 0, imgtype = 0;
	u8 *outbuf = NULL;
	u8 *pngbuf = NULL;
	u8 *finalimage = NULL;

	struct stat filestats;

	char *menu_entries[] = {
	"Default image.",
	"Custom image loaded from a PNG on SD.",
	"Delete the image-display file."};

	imgdisp_exists = 1;
	ret = stat("sdmc:/menuhax_imagedisplay.bin", &filestats);
	if(ret==-1)imgdisp_exists = 0;

	printf("This will configure the image displayed on the main-screen when menuhax triggers. When the image-display file isn't loaded successfully by menuhax, it will display junk.\n");
	if(imgdisp_exists)
	{
		printf("The image-display file already exists on SD.\n");
	}
	else
	{
		printf("The image-display file doesn't exist on SD.\n");
	}

	displaymessage_waitbutton();

	display_menu(menu_entries, 2 + imgdisp_exists, &menuindex, "Select an option with the below menu. You can press B to exit without changing anything.");

	if(menuindex==-1)return 0;

	switch(menuindex)
	{
		case 0:
			pngbuf = (u8*)default_imagedisplay_png;
			pngsize = default_imagedisplay_png_size;
			imgtype = 0;
		break;

		case 1:
			printf("Loading PNG from SD...\n");

			ret = archive_getfilesize(SDArchive, "sdmc:/3ds/menuhax_manager/imagedisplay.png", (u32*)&pngsize);
			if(ret!=0)
			{
				printf("Failed to get the filesize of the SD PNG: 0x%08x.\n", (unsigned int)ret);
				return ret;
			}

			pngbuf = malloc(pngsize);
			if(pngbuf==NULL)
			{
				printf("Failed to alloc the PNG buffer with size 0x%08x.\n", (unsigned int)pngsize);
				return 1;
			}

			ret = archive_readfile(SDArchive, "sdmc:/3ds/menuhax_manager/imagedisplay.png", pngbuf, pngsize);
			if(ret!=0)
			{
				printf("Failed to read the SD PNG: 0x%08x.\n", (unsigned int)ret);
				return ret;
			}

			imgtype = 1;

			printf("SD loading finished.\n");

		break;

		case 2:
			unlink("sdmc:/menuhax_imagedisplay.bin");
		return 0;
	}

	printf("Decoding PNG...\n");

	ret = lodepng_decode24(&outbuf, &w, &h, pngbuf, pngsize);
	if(imgtype==1)free(pngbuf);
	if(ret!=0)
	{
		printf("lodepng returned an error: %s\n", lodepng_error_text(ret));
		return ret;
	}

	printf("Decoding finished.\n");

	if(!(w==800 && h==240) && !(w==240 && h==800))
	{
		printf("PNG width and/or height is invalid. 800x240 or 240x800 is required but the PNG is %ux%u.\n", (unsigned int)w, (unsigned int)h);
		return 2;
	}

	finalimage = malloc(0x8ca00);
	if(finalimage==NULL)
	{
		printf("Failed to alloc the finalimage buffer.\n");
		return 1;
	}

	printf("Converting the image to the required format...\n");

	for(x=0; x<w; x++)
	{
		for(y=0; y<h; y++)
		{
			//Convert the image to 240x800 if it's not already those dimensions.
			pos0 = (x*h + (h-1-y)) * 3;
			pos1 = (y*w + x) * 3;
			if(w==240)pos0 = pos1;

			//Copy the pixel data + swap the color components.
			finalimage[pos0 + 2] = outbuf[pos1 + 0];
			finalimage[pos0 + 1] = outbuf[pos1 + 1];
			finalimage[pos0 + 0] = outbuf[pos1 + 2];
		}
	}

	free(outbuf);

	printf("Writing the final image to SD...\n");

	ret = archive_writefile(SDArchive, "sdmc:/menuhax_imagedisplay.bin", finalimage, 0x8ca00);
	if(ret!=0)
	{
		printf("Failed to write the image-display file to SD: 0x%08x.\n", (unsigned int)ret);
	}
	else
	{
		printf("Successfully wrote the file to SD.\n");
	}

	return ret;
}

int main(int argc, char **argv)
{
	Result ret = 0;
	int menuindex = 0;

	char ropbin_filepath[256];

	char *menu_entries[] = {
	"Install",
	"Delete menuhax, clear the normal theme-cache files, and delete all additional menuhax-only extdata files. This will not delete any menuhax cfg files at SD root, use the belows menus for that. If the X button is held while selecting this, just the additional menuhax-only extdata files will be deleted(this can be used to set the theme to 'none' for example).",
	"Configure/check haxx trigger button(s), which can override the default setting.",
	"Configure menuhax main-screen image display.",
	"Install custom theme for when the Home Menu theme-settings menu was entered at least once with menuhax >=v2.0 installed.",
	"Setup a built-in Home Menu 'Basic' color theme, for when the Home Menu theme-settings menu was entered at least once with menuhax >=v2.0 installed."};

	// Initialize services
	gfxInitDefault();

	consoleInit(GFX_BOTTOM, NULL);

	printf("menuhax_manager %s by yellows8.\n", VERSION);

	memset(ropbin_filepath, 0, sizeof(ropbin_filepath));

	ret = amInit();
	if(ret!=0)
	{
		printf("Failed to initialize AM: 0x%08x.\n", (unsigned int)ret);
		if(ret==0xd8e06406)
		{
			printf("The AM service is inaccessible. With the hblauncher-payload this should never happen. This is normal with plain ninjhax v1.x: this app isn't usable from ninjhax v1.x without any further hax.\n");
		}
	}

	if(ret==0)
	{
		filebuffer = (u8*)malloc(0x400000);
		if(filebuffer==NULL)
		{
			printf("Failed to allocate memory.\n");
			ret = -1;
		}
		else
		{
			memset(filebuffer, 0, filebuffer_maxsize);
		}
	}

	if(ret>=0)
	{
		printf("Opening extdata archives...\n");

		ret = open_extdata();
		if(ret==0)
		{
			printf("Finished opening extdata.\n\n");

			while(ret==0)
			{
				display_menu(menu_entries, 6, &menuindex, "This can install Home Menu haxx to the SD card, for booting hblauncher. Select an option with the below menu. You can press the B button to exit.");

				if(menuindex==-1)break;

				consoleClear();

				switch(menuindex)
				{
					case 0:
						ret = install_menuhax(ropbin_filepath);

						if(ret==0)
						{
							printf("Install finished successfully. The following is the filepath which was just now written, you can delete any SD 'ropbinpayload_menuhax_*' file(s) which don't match the following exact filepath: '%s'. Doing so is completely optional. This only applies when menuhax >v1.2 was already installed where it was switched to a different system-version.\n", ropbin_filepath);
						}
						else
						{
							printf("Install failed: 0x%08x.\n", (unsigned int)ret);
						}
					break;

					case 1:
						ret = delete_menuhax();
						if(ret==0)
						{
							printf("Deletion finished successfully.\n");
						}
						else
						{
							printf("Deletion failed: 0x%08x.\n", (unsigned int)ret);
						}
					break;

					case 2:
						ret = setup_sdcfg();

						if(ret==0)
						{
							printf("Configuration finished successfully.\n");
						}
						else
						{
							printf("Configuration failed: 0x%08x.\n", (unsigned int)ret);
						}
					break;

					case 3:
						ret = setup_imagedisplay();

						if(ret==0)
						{
							printf("Configuration finished successfully.\n");
						}
						else
						{
							printf("Configuration failed: 0x%08x.\n", (unsigned int)ret);
						}
					break;

					case 4:
						printf("Enabling persistent themecache...\n");
						ret = menu_enablethemecache_persistent();
						if(ret==0)
						{
							printf("Installing custom-theme...\n");
							ret = sd2themecache("sdmc:/3ds/menuhax_manager/body_LZ.bin", "sdmc:/3ds/menuhax_manager/bgm.bcstm", 1);
						}

						if(ret==0)
						{
							printf("Custom theme installation finished successfully.\n");
						}
						else
						{
							printf("Custom theme installation failed: 0x%08x. If you haven't already done so, you might need to enter the theme-settings menu under Home Menu, while menuhax is installed.\n", (unsigned int)ret);
						}
					break;

					case 5:
						ret = setup_builtin_theme();

						if(ret==0)
						{
							printf("Theme setup finished successfully.\n");
						}
						else
						{
							printf("Theme setup failed: 0x%08x.\n", (unsigned int)ret);
						}
					break;
				}

				if(ret==0)displaymessage_waitbutton();
			}
		}
	}

	free(filebuffer);

	amExit();

	close_extdata();

	if(ret!=0)printf("An error occured, please report this to here if it persists(or comment on an already existing issue if needed), with an image of your 3DS system with the bottom-screen: https://github.com/yellows8/3ds_homemenuhax/issues\n");

	printf("Press the START button to exit.\n");
	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu
	}

	// Exit services
	gfxExit();
	return 0;
}

