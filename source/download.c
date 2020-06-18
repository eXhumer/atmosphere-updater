/*
 * atmosphere-updater - App that allows you to update Atmosphere (& sigpatches) directly on your switch
 * Copyright (C) 2020 eXhumer
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <time.h>
#include <curl/curl.h>
#include <switch.h>

#include "download.h"
#include "menu.h"
#include "util.h"

#define API_AGENT			"atmosphere-updater"
#define DOWNLOAD_BAR_MAX	500

struct MemoryStruct
{
	char *memory;
	size_t size;
	int mode;
};

static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userdata)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userdata;

	char *ptr = (char *)realloc(mem->memory, mem->size + realsize + 1);

	if (ptr == NULL)
	{
		errorBox(350, 250, "Failed to realloc mem");
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

int download_progress(void *p, double dltotal, double dlnow, double ultotal, double ulnow)
{
	if (dltotal <= 0.0) return 0;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	int counter = tv.tv_usec / 100000;

	if (counter % 2 == 0)
	{ 
		printOptionList(0);
		popUpBox(appFonts.fntSmall, 350, 250, SDL_GetColour(white), "Downloading...");
		drawShape(SDL_GetColour(white), 380, 380, DOWNLOAD_BAR_MAX, 30);
		drawShape(SDL_GetColour(faint_blue), 380, 380, (dlnow / dltotal) * DOWNLOAD_BAR_MAX, 30);

		updateRenderer();
	}
	return 0;
}

int downloadFile(const char *url, const char *output, int api_mode)
{
	CURL *curl = curl_easy_init();
	if (curl)
	{
		FILE *fp = fopen(output, "wb");
		if (fp)
		{
			struct MemoryStruct chunk;
			chunk.memory = (char *)malloc(1);
			chunk.size = 0;

			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_USERAGENT, API_AGENT);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

			if (api_mode == OFF)
			{
				curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
				curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, download_progress);
			}

			CURLcode res = curl_easy_perform(curl);

			fwrite(chunk.memory, 1, chunk.size, fp);

			curl_easy_cleanup(curl);
			free(chunk.memory);
			fclose(fp);

			if (res == CURLE_OK) return 0;
		}
		fclose(fp);
	}

	errorBox(350, 250, "Download failed...");
	return 1;
}
