/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/
//Marker Location
//C:\devel\PlusExp-bin\PlusLibData\ConfigFiles\IntelRealSenseToolDefinitions
#include "PlusConfigure.h"
#include "vtkPlusIntelRealSenseTracker.h"

#include "PlusVideoFrame.h"
#include "vtkImageData.h"
#include "vtkImageImport.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPlusDataSource.h"
/*
Experimental code for surface acquisition
#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include <vtkVersion.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkSmartPointer.h>
*/

#include <fstream>
#include <iostream>
#include <set>

#include "pxcsensemanager.h"
#include "pxctracker.h"
#include "pxcsession.h"
#include "pxcprojection.h"

// From FF_ObjectTracking sample
#define ID_DEVICEX   21000

vtkStandardNewMacro(vtkPlusIntelRealSenseTracker);

void StringToWString(std::wstring &ws, const std::string &s)
{
  std::wstring wsTmp(s.begin(), s.end());
  ws = wsTmp;
}

class Model
{
public:
  Model()
  {
    model_filename[0] = '\0';
  }

  Model(pxcCHAR *filename, std::string aToolSourceId)
  {
    wcsncpy_s<1024>(model_filename, filename, 1024);
    toolSourceId = aToolSourceId;
  }

  struct TrackingState
  {
    pxcUID	cosID;
    pxcCHAR friendlyName[256];
    bool	isTracking;
  };

  void addCosID(pxcUID cosID, pxcCHAR *friendlyName)
  {
    TrackingState state;
    state.cosID = cosID;
    wcscpy_s<256>(state.friendlyName, friendlyName);
    state.isTracking = false;

    cosIDs.push_back(state);
  }

  pxcCHAR model_filename[1024];
  std::vector<TrackingState> cosIDs;
  std::string toolSourceId;
};

typedef std::vector<Model>::iterator				TargetIterator;
typedef std::vector<Model::TrackingState>::iterator TrackingIterator;

class vtkPlusIntelRealSenseTracker::vtkInternal
{
public:
  vtkPlusIntelRealSenseTracker *External;

  PXCSession* Session;

  PXCSenseManager* SenseMgr;
  PXCCaptureManager* CaptureMgr;
  PXCProjection* Projection;

  std::vector<Model> Targets;
  pxcCHAR File[1024];
  pxcCHAR CalibrationFile[1024];
  PXCRectI32 Roi;
  PXCTracker *Tracker;

  bool GetDeviceInfo(int deviceIndex, PXCCapture::DeviceInfo& dinfo)
  {
    PXCSession::ImplDesc desc;
    memset(&desc, 0, sizeof(desc));
    desc.group = PXCSession::IMPL_GROUP_SENSOR;
    desc.subgroup = PXCSession::IMPL_SUBGROUP_VIDEO_CAPTURE;
    for (int i = 0, k = ID_DEVICEX;; i++)
    {
      PXCSession::ImplDesc desc1;
      if (this->Session->QueryImpl(&desc, i, &desc1) < PXC_STATUS_NO_ERROR)
      {
        break;
      }
      PXCCapture *capture;
      if (this->Session->CreateImpl<PXCCapture>(&desc1, &capture) < PXC_STATUS_NO_ERROR)
      {
        continue;
      }
      for (int j = 0;; j++)
      {
        if (capture->QueryDeviceInfo(j, &dinfo) < PXC_STATUS_NO_ERROR) break;
        if (dinfo.orientation == PXCCapture::DEVICE_ORIENTATION_REAR_FACING) break;
        if (j == deviceIndex)
        {
          return true;
        }
      }
    }
    return false;
  }


  vtkInternal(vtkPlusIntelRealSenseTracker* external)
    : External(external)
    , Session(NULL)
    , Tracker(NULL)
    , SenseMgr(NULL)
    , CaptureMgr(NULL)
    , Projection(NULL)
  {
  }

  virtual ~vtkInternal()
  {
  }
};

//----------------------------------------------------------------------------
vtkPlusIntelRealSenseTracker::vtkPlusIntelRealSenseTracker()
  : Internal(new vtkInternal(this))
{
#ifdef USE_INTELREALSENSE_TIMESTAMPS
  this->TrackerTimeToSystemTimeSec = 0;
  this->TrackerTimeToSystemTimeComputed = false;
#endif

  this->IsTrackingInitialized = 0;

  // for accurate timing
  this->FrameNumber = 0;

  // PortName for data source is not required because MapFile identifies each tool, therefore we don't need to enable this->RequirePortNameInDeviceSetConfiguration
  
  // No callback function provided by the device, so the data capture thread will be used to poll the hardware and add new items to the buffer
  this->StartThreadForInternalUpdates=true;
  this->AcquisitionRate = 20;
  
  this->CameraCalibrationFile="IntelRealSenseToolDefinitions/CameraCalibration-CreativeSR300.xml";
  this->DeviceName = "Intel(R) RealSense(TM) 3D Camera SR300";
  this->TrackingMethod = TRACKING_3D;
}

//----------------------------------------------------------------------------
vtkPlusIntelRealSenseTracker::~vtkPlusIntelRealSenseTracker() 
{
  if (this->IsTrackingInitialized)
  {
    this->IsTrackingInitialized=false;
  }
}

//----------------------------------------------------------------------------
std::string vtkPlusIntelRealSenseTracker::GetSdkVersion()
{
	return "";
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusIntelRealSenseTracker::Probe()
{  
  if (this->IsTrackingInitialized)
  {
    LOG_ERROR("vtkPlusIntelRealSenseTracker::Probe should not be called while the device is already initialized");
    return PLUS_FAIL;
  }

  std::string cameraCalibrationFilePath = vtkPlusConfig::GetInstance()->GetDeviceSetConfigurationPath(this->CameraCalibrationFile);
  LOG_DEBUG("Use camera calibration file: " << cameraCalibrationFilePath);
  if (!vtksys::SystemTools::FileExists(cameraCalibrationFilePath.c_str(), true))
  {
    LOG_DEBUG("Unable to find IntelRealSenseTracker camera calibration file at: " << cameraCalibrationFilePath);
  }

  this->IsTrackingInitialized=false;

  return PLUS_SUCCESS;
} 

//----------------------------------------------------------------------------
PlusStatus vtkPlusIntelRealSenseTracker::InternalStartRecording()
{
  if (!this->IsTrackingInitialized)
  {
    LOG_ERROR("InternalStartRecording failed: IntelRealSenseTracker has not been initialized");
    return PLUS_FAIL;
  }
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusIntelRealSenseTracker::InternalStopRecording()
{
  // No need to do anything here, as the IntelRealSenseTracker only performs grabbing on request
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusIntelRealSenseTracker::InternalUpdate()
{
  if (!this->IsTrackingInitialized)
  {
    LOG_ERROR("InternalUpdate failed: IntelRealSenseTracker has not been initialized");
    return PLUS_FAIL;
  }

  // Generate a frame number, as the tool does not provide a frame number.
  // FrameNumber will be used in ToolTimeStampedUpdate for timestamp filtering
  ++this->FrameNumber;

  // Setting the timestamp
  const double unfilteredTimestamp = vtkPlusAccurateTimer::GetSystemTime();

#ifdef USE_INTELREALSENSE_TIMESTAMPS
  if (!this->TrackerTimeToSystemTimeComputed)
  {
    const double timeSystemSec = unfilteredTimestamp;
    const double timeTrackerSec = this->MT->mtGetLatestFrameTime();
    this->TrackerTimeToSystemTimeSec = timeSystemSec-timeTrackerSec;
    this->TrackerTimeToSystemTimeComputed = true;
  }
  const double timeTrackerSec = this->MT->mtGetLatestFrameTime();
  const double timeSystemSec = timeTrackerSec + this->TrackerTimeToSystemTimeSec;        
#endif


  if (this->Internal->SenseMgr->AcquireFrame(true) < PXC_STATUS_NO_ERROR)
  {
    LOG_ERROR("AcquireFrame failed");
    return PLUS_FAIL;
  }


  /* Display Results */
  PXCTracker::TrackingValues  trackData;
  int updatedTrackingCount = 0;
  const PXCCapture::Sample *sample = this->Internal->SenseMgr->QueryTrackerSample();
  if (sample)
  {
    LOG_DEBUG("Sample found!")
  }

  vtkSmartPointer<vtkMatrix4x4> leftHandedToRightHanded = vtkSmartPointer<vtkMatrix4x4>::New();
  leftHandedToRightHanded->SetElement(0, 0, 1);
  leftHandedToRightHanded->SetElement(1, 1, 1);
  leftHandedToRightHanded->SetElement(2, 2, -1);

  // Loop over all of the registered targets (COS IDs) and see if they are tracked
  for (TargetIterator targetIter = this->Internal->Targets.begin(); targetIter != this->Internal->Targets.end(); targetIter++)
  {
    for (TrackingIterator iter = targetIter->cosIDs.begin(); iter != targetIter->cosIDs.end(); iter++)
    {
      this->Internal->Tracker->QueryTrackingValues(iter->cosID, trackData);
      vtkSmartPointer<vtkMatrix4x4> transformMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
      if (PXCTracker::IsTracking(trackData.state))
      {
        updatedTrackingCount += (!iter->isTracking) ? 1 : 0;
        iter->isTracking = true;
        double rotationQuat[4] = { trackData.rotation.w, trackData.rotation.x, trackData.rotation.y, trackData.rotation.z };
        double rotationMatrix[3][3] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
        vtkMath::QuaternionToMatrix3x3(rotationQuat, rotationMatrix);
        int columnMap[3] = { 1, 2, 0 };
        double columnSign[3] = { -1, -1, 1 };
        for (int i = 0; i < 3; i++)
        {
          for (int j = 0; j < 3; j++)
          {
            transformMatrix->SetElement(i, j, columnSign[i]*rotationMatrix[i][columnMap[j]]);
          }
        }
        transformMatrix->SetElement(0, 3, trackData.translation.x);
        transformMatrix->SetElement(1, 3, trackData.translation.y);
        transformMatrix->SetElement(2, 3, trackData.translation.z);
        vtkMatrix4x4::Multiply4x4(leftHandedToRightHanded, transformMatrix, transformMatrix);
        vtkMatrix4x4::Multiply4x4(transformMatrix, leftHandedToRightHanded, transformMatrix);
      }
      else
      {
        updatedTrackingCount += iter->isTracking ? 1 : 0;
        iter->isTracking = false;
      }

#ifdef USE_INTELREALSENSE_TIMESTAMPS
      ToolTimeStampedUpdateWithoutFiltering(targetIter->toolSourceId.c_str(), transformMatrix, iter->isTracking?TOOL_OK:TOOL_OUT_OF_VIEW, timeSystemSec, timeSystemSec);
#else
      ToolTimeStampedUpdate(targetIter->toolSourceId.c_str(), transformMatrix, iter->isTracking ? TOOL_OK : TOOL_OUT_OF_VIEW, this->FrameNumber, unfilteredTimestamp);
#endif

    }
  }

  /*
  Experimental code for getting surface mesh
  colorImage = sample->color;
  depthImage = sample->depth;
  width = 640;
  height = 480;

  PXCProjection::QueryVertices(PXCImage *depthImage, PXCPoint3DF32 *vertices);

  vtkPolyData* polydata = vtkPolyData::New();
  vtkCellArray* strips = vtkCellArray::New();
  //need to reference hight and width of image
  for (int col = 0; col < projection->height; col++)
  { 
	  strips->InsertNextCell(projection->width - 1);
	  for (int row = 0; row < width; row++)
	  {
		  for (int k = 0; k < 2; k++)
		  {
			  strips->InsertCellPoint(Projection->Vertices[col*(projection->width) + row]);
			  strips->InsertCellPoint(Projection->Vertices[(col + 1)*(projection->width) + row]);
		  }
	  }

  }
  polydata->SetStrips(strips);

  vtkSmartPointer<vtkXMLPolyDataWriter> writer = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  writer->SetFileName("PolyDataFile.vtp");
  writer->SetInputData(polydata);
  writer->Write();
  */


  this->Internal->SenseMgr->ReleaseFrame();

  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusIntelRealSenseTracker::GetImage(vtkImageData* leftImage, vtkImageData* rightImage)
{
  PlusLockGuard<vtkPlusRecursiveCriticalSection> updateMutexGuardedLock(this->UpdateMutex);
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusIntelRealSenseTracker::ReadConfiguration( vtkXMLDataElement* rootConfigElement )
{
  XML_FIND_DEVICE_ELEMENT_REQUIRED_FOR_READING(deviceConfig, rootConfigElement);
  XML_READ_CSTRING_ATTRIBUTE_OPTIONAL(CameraCalibrationFile, deviceConfig);
  XML_READ_CSTRING_ATTRIBUTE_OPTIONAL(DeviceName, deviceConfig);
  XML_READ_ENUM2_ATTRIBUTE_OPTIONAL(TrackingMethod, deviceConfig, "3D", TRACKING_3D, "2D", TRACKING_2D);

  for (DataSourceContainerConstIterator it = this->GetToolIteratorBegin(); it != this->GetToolIteratorEnd(); ++it)
  {
    vtkPlusDataSource *tool = it->second;
  }

  XML_FIND_NESTED_ELEMENT_REQUIRED(dataSourcesElement, deviceConfig, "DataSources");
  for (int nestedElementIndex = 0; nestedElementIndex < dataSourcesElement->GetNumberOfNestedElements(); nestedElementIndex++)
  {
    vtkXMLDataElement* toolDataElement = dataSourcesElement->GetNestedElement(nestedElementIndex);
    if (STRCASECMP(toolDataElement->GetName(), "DataSource") != 0)
    {
      // if this is not a data source element, skip it
      continue;
    }
    if (toolDataElement->GetAttribute("Type") != NULL && STRCASECMP(toolDataElement->GetAttribute("Type"), "Tool") != 0)
    {
      // if this is not a Tool element, skip it
      continue;
    }
    const char* toolId = toolDataElement->GetAttribute("Id");
    PlusTransformName toolTransformName(toolId, this->GetToolReferenceFrameName());
    std::string toolSourceId = toolTransformName.GetTransformName();
    if (toolId == NULL)
    {
      LOG_ERROR("Failed to initialize NDI tool: DataSource Id is missing");
      continue;
    }
    if (toolDataElement->GetAttribute("MapFile") != NULL)
    {
      std::string modelAttributeName;
      switch (this->TrackingMethod)
      {
      case TRACKING_2D:
        modelAttributeName = "MarkerImageFile"; // .png file
        break;
      case TRACKING_3D:
      default:
        modelAttributeName = "MapFile"; // .slam file
      }
      std::string modelFile = toolDataElement->GetAttribute(modelAttributeName.c_str());
      std::string modelFilePath = vtkPlusConfig::GetInstance()->GetDeviceSetConfigurationPath(modelFile);
      std::wstring modelFilePathW;
      StringToWString(modelFilePathW, modelFilePath);
      this->Internal->Targets.push_back(Model((pxcCHAR*)modelFilePathW.c_str(), toolSourceId));
    }
  }
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusIntelRealSenseTracker::WriteConfiguration(vtkXMLDataElement* rootConfigElement)
{
  XML_FIND_DEVICE_ELEMENT_REQUIRED_FOR_WRITING(trackerConfig, rootConfigElement);

  trackerConfig->SetAttribute("CameraCalibrationFile", this->CameraCalibrationFile.c_str()); 
  trackerConfig->SetAttribute("DeviceName", this->DeviceName.c_str());
  // TODO: write/update TrackingMethod
  return PLUS_SUCCESS;
} 

//----------------------------------------------------------------------------
PlusStatus vtkPlusIntelRealSenseTracker::InternalConnect()
{ 
  if (this->IsTrackingInitialized)
  {
    LOG_DEBUG("Already connected to IntelRealSenseTracker");
    return PLUS_SUCCESS;
  }

  this->Internal->Session = PXCSession::CreateInstance();
  if (!this->Internal->Session)
  {
    LOG_ERROR("Failed to create an SDK session");
    return PLUS_FAIL;
  }
  
  this->Internal->SenseMgr = this->Internal->Session->CreateSenseManager();
  if (!this->Internal->SenseMgr)
  {
    LOG_ERROR("Failed to create an SDK SenseManager");
    return PLUS_FAIL;
  }
  
  PXCSession *session = this->Internal->SenseMgr->QuerySession();
  PXCSession::CoordinateSystem cs = session->QueryCoordinateSystem();

  /* Set Mode & Source */
  pxcStatus sts = PXC_STATUS_NO_ERROR;
  this->Internal->CaptureMgr = this->Internal->SenseMgr->QueryCaptureManager(); //no need to Release it is released with senseMgr
  // Live streaming
  PXCCapture::DeviceInfo dinfo;
  this->Internal->GetDeviceInfo(0, dinfo);
  pxcCHAR* device = dinfo.name;
  this->Internal->CaptureMgr->FilterByDeviceInfo(device, 0, 0);
  bool stsFlag = true;

  /* Set Module */
  sts = this->Internal->SenseMgr->EnableTracker();
  if (sts < PXC_STATUS_NO_ERROR)
  {
    LOG_ERROR("Failed to enable tracking module");
    return PLUS_FAIL;
  }

  // Init
  LOG_DEBUG("Init Started");

  this->Internal->Tracker = this->Internal->SenseMgr->QueryTracker();
  if (this->Internal->Tracker == NULL)
  {
    LOG_ERROR("Failed to Query tracking module");
    return PLUS_FAIL;
  }

  if (!this->CameraCalibrationFile.empty())
  {
    std::string cameraCalibrationFilePath = vtkPlusConfig::GetInstance()->GetDeviceSetConfigurationPath(this->CameraCalibrationFile);
    LOG_DEBUG("Use IntelRealSenseTracker ini file: " << cameraCalibrationFilePath);
    if (!vtksys::SystemTools::FileExists(cameraCalibrationFilePath.c_str(), true))
    {
      LOG_WARNING("Unable to find IntelRealSenseTracker camera calibration file at: " << cameraCalibrationFilePath);
    }
    std::wstring cameraCalibrationFileW;
    StringToWString(cameraCalibrationFileW, cameraCalibrationFilePath);
    if (this->Internal->Tracker->SetCameraParameters(cameraCalibrationFileW.c_str()) != PXC_STATUS_NO_ERROR)
    {
      LOG_WARNING("Warning: failed to load camera calibration");
    }
  }

  if (this->Internal->SenseMgr->Init() < PXC_STATUS_NO_ERROR)
  {
    LOG_ERROR("senseMgr->Init failed");
    return PLUS_FAIL;
  }

  this->Internal->Projection = this->Internal->SenseMgr->QueryCaptureManager()->QueryDevice()->CreateProjection();
  pxcUID cosID;
  for (size_t i = 0; i < this->Internal->Targets.size(); i++)
  {
    this->Internal->Targets[i].cosIDs.clear();
    switch (this->TrackingMethod)
    {
      case TRACKING_3D:
      {
        pxcUID firstID, lastID;
        sts = this->Internal->Tracker->Set3DTrack(this->Internal->Targets[i].model_filename, firstID, lastID);
        while (firstID <= lastID)
        {
          PXCTracker::TrackingValues vals;
          this->Internal->Tracker->QueryTrackingValues(firstID, vals);
          this->Internal->Targets[i].addCosID(firstID, vals.targetName);
          firstID++;
        }
        break;
      }
      case TRACKING_2D:
      {
        sts = this->Internal->Tracker->Set2DTrackFromFile(this->Internal->Targets[i].model_filename, cosID);
        this->Internal->Targets[i].addCosID(cosID, L"2D Image");
        break;
      }
      default:
      {
        LOG_ERROR("vtkPlusIntelRealSenseTracker::InternalConnect failed: unknown tracking method " << this->TrackingMethod);
        return PLUS_FAIL;
      }
    }

    if (sts < PXC_STATUS_NO_ERROR)
    {
      LOG_ERROR("Failed to set tracking configuration");
      return PLUS_FAIL;
    }
  }

  this->IsTrackingInitialized=1;

  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusIntelRealSenseTracker::InternalDisconnect()
{ 
  if (this->IsTrackingInitialized)
  {
    if (this->Internal->Projection)
    {
      this->Internal->Projection->Release();
      this->Internal->Projection = NULL;
    }
    this->Internal->SenseMgr->Close();
    this->Internal->SenseMgr->Release();
    this->IsTrackingInitialized=false;
  }  
  return PLUS_SUCCESS;
}
