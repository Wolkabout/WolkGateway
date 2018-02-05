/*
 * Copyright 2018 WolkAbout Technology s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FIRMWAREUPDATESERVICE_H
#define FIRMWAREUPDATESERVICE_H

#include "FirmwareUpdateCommandListener.h"
#include "utilities/ByteUtils.h"
#include "utilities/CommandBuffer.h"
#include "WolkaboutFileDownloader.h"
#include "UrlFileDownloader.h"
#include <string>
#include <memory>
#include <cstdint>

namespace wolkabout
{
class OutboundServiceDataHandler;
class FirmwareInstaller;
class FirmwareUpdateResponse;

class FirmwareUpdateService: public FirmwareUpdateCommandListener
{
public:
	FirmwareUpdateService(const std::string& firmwareVersion, const std::string& downloadDirectory,
						  std::uint_fast64_t maximumFirmwareSize,
						  std::shared_ptr<OutboundServiceDataHandler> outboundDataHandler,
						  std::weak_ptr<WolkaboutFileDownloader> wolkDownloader,
						  std::weak_ptr<UrlFileDownloader> urlDownloader,
						  std::weak_ptr<FirmwareInstaller> firmwareInstaller);

	~FirmwareUpdateService();

	FirmwareUpdateService (const FirmwareUpdateService&) = delete;
	FirmwareUpdateService& operator= (const FirmwareUpdateService&) = delete;

	void handleFirmwareUpdateCommand(const FirmwareUpdateCommand& firmwareUpdateCommand) override;

	const std::string& getFirmwareVersion() const;

	void reportFirmwareUpdateResult();

private:
	void addToCommandBuffer(std::function<void()> command);

	void sendResponse(const FirmwareUpdateResponse& response);

	void onFirmwareFileDownloadSuccess(const std::string& filePath);

	void onFirmwareFileDownloadFail(WolkaboutFileDownloader::Error errorCode);

	void onFirmwareFileDownloadFail(UrlFileDownloader::Error errorCode);

	void downloadFirmware(const std::string& name, std::uint_fast64_t size, const ByteArray& hash);

	void downloadFirmware(const std::string& url);

	void install();

	void clear();

	class FirmwareUpdateServiceState
	{
	public:
		FirmwareUpdateServiceState(FirmwareUpdateService& service): m_service{service} {}
		virtual void handleFirmwareUpdateCommand(const FirmwareUpdateCommand& firmwareUpdateCommand) = 0;
		virtual ~FirmwareUpdateServiceState() = default;
	protected:
		FirmwareUpdateService& m_service;
	};

	class IdleState: public FirmwareUpdateServiceState
	{
	public:
		using FirmwareUpdateServiceState::FirmwareUpdateServiceState;
		void handleFirmwareUpdateCommand(const FirmwareUpdateCommand& firmwareUpdateCommand) override;
	};

	class WolkDownloadState: public FirmwareUpdateServiceState
	{
	public:
		using FirmwareUpdateServiceState::FirmwareUpdateServiceState;
		void handleFirmwareUpdateCommand(const FirmwareUpdateCommand& firmwareUpdateCommand) override;
	};

	class UrlDownloadState: public FirmwareUpdateServiceState
	{
	public:
		using FirmwareUpdateServiceState::FirmwareUpdateServiceState;
		void handleFirmwareUpdateCommand(const FirmwareUpdateCommand& firmwareUpdateCommand) override;
	};

	class ReadyState: public FirmwareUpdateServiceState
	{
	public:
		using FirmwareUpdateServiceState::FirmwareUpdateServiceState;
		void handleFirmwareUpdateCommand(const FirmwareUpdateCommand& firmwareUpdateCommand) override;
	};

	class InstallationState: public FirmwareUpdateServiceState
	{
	public:
		using FirmwareUpdateServiceState::FirmwareUpdateServiceState;
		void handleFirmwareUpdateCommand(const FirmwareUpdateCommand& firmwareUpdateCommand) override;
	};

	friend class IdleState;
	friend class WolkDownloadState;
	friend class UrlDownloadState;
	friend class ReadyState;

	const std::string m_currentFirmwareVersion;
	const std::string m_firmwareDownloadDirectory;
	const std::uint_fast64_t m_maximumFirmwareSize;

	std::shared_ptr<OutboundServiceDataHandler> m_outboundDataHandler;
	std::weak_ptr<WolkaboutFileDownloader> m_wolkFileDownloader;
	std::weak_ptr<UrlFileDownloader> m_urlFileDownloader;
	std::weak_ptr<FirmwareInstaller> m_firmwareInstaller;

	std::unique_ptr<IdleState> m_idleState;
	std::unique_ptr<WolkDownloadState> m_wolkDownloadState;
	std::unique_ptr<UrlDownloadState> m_urlDownloadState;
	std::unique_ptr<ReadyState> m_readyState;
	std::unique_ptr<InstallationState> m_installationState;

	FirmwareUpdateServiceState* m_currentState;

	std::string m_firmwareFile;
	bool m_autoInstall;

	std::unique_ptr<std::thread> m_executor;

	std::unique_ptr<CommandBuffer> m_commandBuffer;

	static const constexpr char* FIRMWARE_VERSION_FILE = ".dfu-version";
};

}

#endif // FIRMWAREUPDATESERVICE_H
