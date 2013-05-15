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

#ifndef SERVICEFEEDBACKCODE_H
#define SERVICEFEEDBACKCODE_H

#include "Common/NETypes.h"

namespace Isa100 {
namespace Common {
namespace Objects {

namespace SFC {
/**
 * Service feedback code is used to indicate status (e.g., success), warning (e.g., value limited), or error (e.g., incompatible mode).
 */
enum SFCEnum {
    success = 0, failure = 1, // generic failure
    other = 2, // reason other than that listed in this enumeration
    invalidArgument = 3, // invalid attribute to a service call
    invalidObjectID = 4, // invalid object ID
    invalidService = 5, // unsupported or illegal service
    invalidAtrribute = 6, // invalid attribute index
    invalidElementIndex = 7, // invalid array or structure element index (or indices)
    readOnlyAtrribute = 8, // read-only attribute
    valueOutOfRange = 9, // value is out of permitted range
    inappropriateProcessMode = 10, // process is in an inappropriate mode for the request
    incompatibleMode = 11, // value is not acceptable in current context
    invalidValue = 12, // value (data) not acceptable for other reason (e.g., too large, too small,
    // invalid engineering units code)
    internalError = 13, // device internal problem
    invalidSize = 14, // size is not valid (may be too big or too small)
    incompatibleAttribute = 15, // attribute not supported in this version
    invalidMethod = 16, // invalid method identifier
    objectStateConflict = 17, // state of object in conflict with action requested
    inconsistentContent = 18, // the content of the service requested is inconsistent
    invalidParameter = 19, // value conveyed is not legal for method invocation
    objectAccessDenied = 20, // object is not permitting access
    typeMismatch = 21, // data not as expected (e.g., too many or too few octets)
    deviceHardwareCondition = 22, // device specific hardware condition prevented request from
    // succeeding (e.g., memory defect problem)
    deviceSensorCondition = 23, // problem with sensor detected
    deviceSoftwareCondition = 24, // device specific software condition prevented request from  succeeding
    // (e.g., local lockout, local write protection, simulating in progress)
    fieldOperationCondition = 25, // field specific condition prevented request from succeeding
    // (e.g., lockout, or environmental condition not in range)
    configurationMismatch = 26, // a configuration conflict was detected
    insufficientDeviceResources = 27, // e.g., queue full, buffers / memory unavailable
    valueLimited = 28, // e.g., value limited by device
    dataWarning = 29, // e.g., value has been modified due to a device specific reason
    invalidFunctionReference = 30, // function referenced for execution is invalid
    functionProcessError = 31, // function referenced could not be performed due to a device specific reason
    warning = 32, // successful, the there is additional information that may be of interest to the user.
    // Such additional may, for example be conveyed via accessing an attribute, or by sending an alert.
    writeOnlyAttribute = 33, // write-only attribute (e.g., a command attribute)
    operationAccepted = 34, // method operation accepted
    invalidBlockSize = 35, // upload or download block size not valid
    invalidDownloadSize = 36, // total size for upload not valid
    unexpectedMethodSequence = 37, // required method sequencing has not been followed
    timingViolation = 38, // object timing requirements have not been satisfied
    operationIncomplete = 39, // method operation, or method operation sequence not successful
    invalidData = 40, // data received is not valid (e.g., checksum error, data content not as expected, etc.)
    dataSequenceError = 41, // data is ordered; data received is not in the order required
    // example: duplicate data was received
    operationAborted = 42, // operation aborted by server
    invalidBlockNumber = 43,
    blockDataError = 44, //error in block of data, example, wrong size, invalid content
    blockNotDownloaded = 45, // the specified block of data has not been successfully downloaded

    // range 46 through 127 are reserved for future use of this standard

    // vendor-specific device-specific feedback codes, range 128 - 255

    vendorDefinedError_128 = 128,
    timeout = 129,
    deviceNotFound = 130,
    duplicate = 131, // add a contract to device, the contract already exists => the device responds with error code: duplicate
    sent = 132, //fake code : used by IsaOperation to mark the mapping RequiestID = Status sent..can be anithing other than success
    vendorDefinedError_254 = 254,
    extensionCode = 255
// indicates a two octet field size follows for an extended service feedback code value

}; //end enum ServiceFeedbackCode

inline std::string getSFCDescription(SFCEnum sfcCode) {
    if (sfcCode == SFC::success) {
        return "success";
    } else if (sfcCode == SFC::failure) {
        return "failure";
    } else if (sfcCode == SFC::other) {
        return "other";
    } else if (sfcCode == SFC::invalidArgument) {
        return "invalidArgument";
    } else if (sfcCode == SFC::invalidObjectID) {
        return "invalidObjectID";
    } else if (sfcCode == SFC::invalidService) {
        return "invalidService";
    } else if (sfcCode == SFC::invalidAtrribute) {
        return "invalidAtrribute";
    } else if (sfcCode == SFC::invalidElementIndex) {
        return "invalidElementIndex";
    } else if (sfcCode == SFC::readOnlyAtrribute) {
        return "readOnlyAtrribute";
    } else if (sfcCode == SFC::valueOutOfRange) {
        return "valueOutOfRange";
    } else if (sfcCode == SFC::inappropriateProcessMode) {
        return "inappropriateProcessMode";
    } else if (sfcCode == SFC::incompatibleMode) {
        return "incompatibleMode";
    } else if (sfcCode == SFC::invalidValue) {
        return "invalidValue";
    } else if (sfcCode == SFC::internalError) {
        return "internalError";
    } else if (sfcCode == SFC::invalidSize) {
        return "invalidSize";
    } else if (sfcCode == SFC::incompatibleAttribute) {
        return "incompatibleAttribute";
    } else if (sfcCode == SFC::invalidMethod) {
        return "invalidMethod";
    } else if (sfcCode == SFC::objectStateConflict) {
        return "objectStateConflict";
    } else if (sfcCode == SFC::inconsistentContent) {
        return "inconsistentContent";
    } else if (sfcCode == SFC::invalidParameter) {
        return "invalidParameter";
    } else if (sfcCode == SFC::objectAccessDenied) {
        return "objectAccessDenied";
    } else if (sfcCode == SFC::typeMismatch) {
        return "typeMismatch";
    } else if (sfcCode == SFC::deviceHardwareCondition) {
        return "deviceHardwareCondition";
    } else if (sfcCode == SFC::deviceSensorCondition) {
        return "deviceSensorCondition";
    } else if (sfcCode == SFC::deviceSoftwareCondition) {
        return "deviceSoftwareCondition";
    } else if (sfcCode == SFC::fieldOperationCondition) {
        return "fieldOperationCondition";
    } else if (sfcCode == SFC::configurationMismatch) {
        return "configurationMismatch";
    } else if (sfcCode == SFC::insufficientDeviceResources) {
        return "insufficientDeviceResources";
    } else if (sfcCode == SFC::valueLimited) {
        return "valueLimited";
    } else if (sfcCode == SFC::dataWarning) {
        return "dataWarning";
    } else if (sfcCode == SFC::invalidFunctionReference) {
        return "invalidFunctionReference";
    } else if (sfcCode == SFC::functionProcessError) {
        return "functionProcessError";
    } else if (sfcCode == SFC::warning) {
        return "warning";
    } else if (sfcCode == SFC::writeOnlyAttribute) {
        return "writeOnlyAttribute";
    } else if (sfcCode == SFC::operationAccepted) {
        return "operationAccepted";
    } else if (sfcCode == SFC::invalidBlockSize) {
        return "invalidBlockSize";
    } else if (sfcCode == SFC::invalidDownloadSize) {
        return "invalidDownloadSize";
    } else if (sfcCode == SFC::unexpectedMethodSequence) {
        return "unexpectedMethodSequence";
    } else if (sfcCode == SFC::timingViolation) {
        return "timingViolation";
    } else if (sfcCode == SFC::operationIncomplete) {
        return "operationIncomplete";
    } else if (sfcCode == SFC::invalidData) {
        return "invalidData";
    } else if (sfcCode == SFC::dataSequenceError) {
        return "dataSequenceError";
    } else if (sfcCode == SFC::operationAborted) {
        return "operationAborted";
    } else if (sfcCode == SFC::invalidBlockNumber) {
        return "invalidBlockNumber";
    } else if (sfcCode == SFC::blockDataError) {
        return "blockDataError";
    } else if (sfcCode == SFC::blockNotDownloaded) {
        return "blockNotDownloaded";
    } else if (sfcCode == SFC::vendorDefinedError_128) {
        return "vendorDefinedError_128";
    } else if (sfcCode == SFC::timeout) {
        return "timeout";
    } else if (sfcCode == SFC::deviceNotFound) {
        return "deviceNotFound";
    } else if (sfcCode == SFC::duplicate) {
        return "duplicate";
    } else if (sfcCode == SFC::sent) {
        return "sent";
    } else if (sfcCode == SFC::vendorDefinedError_254) {
        return "vendorDefinedError_254";
    } else if (sfcCode == SFC::extensionCode) {
        return "extensionCode";
    }

    std::ostringstream stream;
    stream << "N/A ";
    stream << sfcCode;
    return stream.str();
}

} // end namespace SFC

} // end namespace Types
} // end namespace ASL
} // namespace Isa100

#endif
