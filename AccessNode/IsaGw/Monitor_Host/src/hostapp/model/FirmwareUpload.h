/*
* Copyright (C) 2013 Nivis LLC.
* Email:   opensource@nivis.com
* Website: http://www.nivis.com
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, version 3 of the License.
* 
* Redistribution and use in source and binary forms must retain this
* copyright notice.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#ifndef FIRMWAREUPLOAD_H_
#define FIRMWAREUPLOAD_H_

namespace nisa100 {
namespace hostapp {
#include <vector>


class FirmwareUpload;
typedef std::vector<FirmwareUpload> FirmwareUploadsT;

class FirmwareUpload
{
public:
	enum FirmwareUploadStatus
	{
		fusNew = 0,
		fusSuccess = 1,
		fusUploading = 2,
		fusWaitRetry = 3,
		fusFailed = 4,
		//comment - this is only used to allow NMT to change the name of file already inserted in db
		fusNameInProgress = 5
	};


	FirmwareUploadStatus Status;
	boost::uint32_t Id;
	int RetriesCount;
	nlib::DateTime LastFailedUploadTime;

	std::string FileName;

	const std::string ToString() const
	{
		return boost::str(boost::format("FirmwareUpload[Id=%1% Status=%2% RetriesCount=%3% FileName=<%4%> ")
			% Id % (int)Status % RetriesCount % FileName.c_str());

	}
};
}
}

#endif /*FIRMWAREUPLOAD_H_*/
