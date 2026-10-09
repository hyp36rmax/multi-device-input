#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#include <filesystem>
#include <fstream>
#include <string>
#include "resource.h"

namespace Util {

std::string HttpGetRequest(const std::string& host, const std::wstring& path, int portNum = 80)
{
	HINTERNET hSession = WinHttpOpen(L"OutRun2006Tweaks/" MODULE_VERSION_STR,
		WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession)
		return "";

	HINTERNET hConnect = WinHttpConnect(hSession, std::wstring(host.begin(), host.end()).c_str(), portNum, 0);
	if (!hConnect)
	{
		WinHttpCloseHandle(hSession);
		return "";
	}

	HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
		NULL, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES,
		portNum == INTERNET_DEFAULT_HTTPS_PORT ? WINHTTP_FLAG_SECURE : 0);
	if (!hRequest)
	{
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return "";
	}

	BOOL bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
		WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

	if (bResults)
		bResults = WinHttpReceiveResponse(hRequest, NULL);

	std::string response;
	if (bResults)
	{
		DWORD dwSize = 0;
		do
		{
			DWORD dwDownloaded = 0;
			if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
				break;

			if (dwSize == 0)
				break;

			char* buffer = new char[dwSize + 1];
			if (!WinHttpReadData(hRequest, buffer, dwSize, &dwDownloaded))
			{
				delete[] buffer;
				break;
			}

			buffer[dwDownloaded] = '\0';
			response.append(buffer, dwDownloaded);
			delete[] buffer;
		} while (dwSize > 0);
	}

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	return response;
}

bool HttpDownloadFile(const std::string& host, const std::wstring& path,
	const std::filesystem::path& destination, int portNum)
{
	HINTERNET session = WinHttpOpen(L"HYP36Rforce/" MODULE_VERSION_STR,
		WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);
	if (!session)
		return false;

	HINTERNET connection = WinHttpConnect(session,
		std::wstring(host.begin(), host.end()).c_str(), portNum, 0);
	HINTERNET request = connection ? WinHttpOpenRequest(connection, L"GET", path.c_str(),
		nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
		portNum == INTERNET_DEFAULT_HTTPS_PORT ? WINHTTP_FLAG_SECURE : 0) : nullptr;

	bool success = request && WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
		WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(request, nullptr);
	DWORD status = 0;
	DWORD statusSize = sizeof(status);
	if (success)
		success = WinHttpQueryHeaders(request,
			WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
			WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX) &&
			status >= 200 && status < 300;

	std::ofstream output;
	if (success)
	{
		output.open(destination, std::ios::binary | std::ios::trunc);
		success = bool(output);
	}

	while (success)
	{
		DWORD available = 0;
		if (!WinHttpQueryDataAvailable(request, &available))
		{
			success = false;
			break;
		}
		if (available == 0)
			break;

		std::string buffer(available, '\0');
		DWORD downloaded = 0;
		if (!WinHttpReadData(request, buffer.data(), available, &downloaded))
		{
			success = false;
			break;
		}
		output.write(buffer.data(), downloaded);
		success = bool(output);
	}

	output.close();
	if (request) WinHttpCloseHandle(request);
	if (connection) WinHttpCloseHandle(connection);
	WinHttpCloseHandle(session);
	if (!success)
	{
		std::error_code error;
		std::filesystem::remove(destination, error);
	}
	return success;
}

};
