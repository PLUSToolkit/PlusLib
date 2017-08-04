/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/

#ifndef _vtkUSDigitalA2EncodersTracker_h_
#define _vtkUSDigitalA2EncodersTracker_h_

#include "vtkPlusDataCollectionExport.h"
#include "vtkPlusDevice.h"
#include "vtkPlusTransformRepository.h"

#include <map>

/*!
   \class vtkPlusUSDigitalEncodersTracker
   \brief Interface for multiple US Digital A2, A2T, A4, HBA2, HBA4 or HD25A encoders to generate pose information of a target object.

   This class communicates with multiple US Digital encoders through SEI (Serial Encoder Interface Bus) provided by US Digital.

   \ingroup PlusLibDataCollection
 */

class vtkPlusDataCollectionExport vtkPlusUSDigitalEncodersTracker : public vtkPlusDevice
{
public:
  static const long INVALID_SEI_ADDRESS = -1;

  static vtkPlusUSDigitalEncodersTracker* New();
  vtkTypeMacro(vtkPlusUSDigitalEncodersTracker, vtkPlusDevice);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual bool IsTracker() const
  {
    return true;
  }

  /*! Connect to device */
  PlusStatus InternalConnect();

  /*! Disconnect from device */
  virtual PlusStatus InternalDisconnect();

  /*! Read main configuration from xml data */
  virtual PlusStatus ReadConfiguration(vtkXMLDataElement* config);

  /*! Write main configuration to xml data */
  virtual PlusStatus WriteConfiguration(vtkXMLDataElement* config);

  /*!
     Probe to see if the tracking system is present on the specified serial port.
     If the SerialPort is set to -1, then all serial ports will be checked.
   */
  PlusStatus Probe();

  /*!
     Get an update from the multiple USDigital encoders and push the new transforms
     to the tools.  This should only be used within vtkTracker.cxx.
     This method is called by the tracker thread.
   */
  PlusStatus InternalUpdate();

  vtkGetMacro(NumberOfEncoders, long);
  vtkSetMacro(NumberOfEncoders, long);

  /*! Return whether stepper is alive */
  virtual PlusStatus IsStepperAlive();

  /*! If the A2 is in strobe mode, it will take a position reading after receiving this command.*/
  PlusStatus SetUSDigitalA2EncodersStrobeMode();

  /*! Makes all A2's on the SEI bus go to sleep, the current consumption then drops below 0.6 mA / device */
  PlusStatus SetUSDigitalA2EncodersSleep();

  /*! Function: wakes up all A2's on the SEI bus, wait at least 5mSec before sending the next command */
  PlusStatus SetUSDigitalA2EncodersWakeup();

  /*! Sets the absolute zero to the current position, in single-turn mode the new position is stored in EEPROM
      address: SEI address 0-15*/
  PlusStatus SetUSDigitalA2EncoderOriginWithAddr(long address);

  /*! Sets the absolute zero to the current position of all connected US Digital A2 Encoders*/
  PlusStatus SetAllUSDigitalA2EncoderOrigin();

  /*! Sets the mode of an A2 Encoder
      address: SEI address 0-14*/
  PlusStatus SetUSDigitalA2EncoderModeWithAddr(long address, long mode);

  /*! Gets the mode of an A2 Encoder
      address: SEI address 0-14*/
  PlusStatus GetUSDigitalA2EncoderModeWithAddr(long address, long* mode);

  /*! Sets the resolution of an A2 Encoder
      address: SEI address 0-14*/
  PlusStatus SetUSDigitalA2EncoderResoultionWithAddr(long address, long res);

  /*! Gets the resolution of an A2 Encoder
      address: SEI address 0-14*/
  PlusStatus GetUSDigitalA2EncoderResoultionWithAddr(long address, long* res);

  /*! Sets the Position of an A2 Encoder
      address: SEI address 0-14*/
  PlusStatus SetUSDigitalA2EncoderPositionWithAddr(long address, long pos);

  /*! Gets the Position of an A2 Encoder
      address: SEI address 0-14*/
  PlusStatus GetUSDigitalA2EncoderPositionWithAddr(long address, long* pos);

protected:
  vtkPlusUSDigitalEncodersTracker();
  ~vtkPlusUSDigitalEncodersTracker();

  /*!
     Start the tracking system.  The tracking system is brought from its ground state into full tracking mode.
     The device will only be reset if communication cannot be established without a reset.
   */
  PlusStatus InternalStartRecording();

  /*! Stop the tracking system and bring it back to its ground state: Initialized, not tracking, at 9600 Baud. */
  PlusStatus InternalStopRecording();

  bool IsValidSEIAddress(long address);

protected:
  bool CoreXY = false;
  /*! Number of connected A2 Encoders */
  long NumberOfEncoders = 0;
  long COMPort = 0;

  class vtkPlusEncoderTrackingInfo;
  class vtkPlusUSDigitalEncoderInfo;

  vtkSmartPointer<vtkPlusTransformRepository> USDigitalEncoderTransformRepository
    = vtkSmartPointer<vtkPlusTransformRepository>::New();

  typedef std::map<long, vtkPlusUSDigitalEncoderInfo*> EncoderInfoMapType;
  EncoderInfoMapType USDigitalEncoderInfoList;

  typedef std::vector<vtkPlusEncoderTrackingInfo> EncoderTrackingInfoVectorType;
  EncoderTrackingInfoVectorType USDigitalEncoderTrackingInfoList;


private:  // Functions.
  vtkPlusUSDigitalEncodersTracker(const vtkPlusUSDigitalEncodersTracker&);
  void operator=(const vtkPlusUSDigitalEncodersTracker&);
  PlusStatus ToolTimeStampedUpdateWithvtkPlusEncoderTrackingInfo(vtkPlusEncoderTrackingInfo& encoderTrackingInfo);
};

#endif