/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/

#include "PlusConfigure.h"

#include "vtkAscension3DGTracker.h"

#include <sstream>

#ifdef PLUS_USE_Ascension3DGm
#include "ATC3DGm.h"
#define ATC
SENSOR_CONFIGURATION* pSensor=NULL;
#else /* PLUS_USE_Ascension3DGm */
namespace atc
{
#include "ATC3DG.h"
}
#define ATC atc
#endif /* PLUS_USE_Ascension3DGm */



#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtksys/SystemTools.hxx"
#include "vtkTransform.h"
#include "vtkXMLDataElement.h"

#include "PlusConfigure.h"
#include "vtkTracker.h"
#include "vtkTrackerTool.h"
#include "vtkTrackerBuffer.h"

vtkStandardNewMacro(vtkAscension3DGTracker);
typedef ATC::DOUBLE_POSITION_ANGLES_MATRIX_QUATERNION_TIME_Q_BUTTON_RECORD AscensionRecordType;

//-------------------------------------------------------------------------
vtkAscension3DGTracker::vtkAscension3DGTracker()
{
  this->LocalTrackerBuffer = NULL;
  this->AscensionRecordBuffer = NULL; 

  this->TransmitterAttached = false;
  this->NumberOfSensors = 0; 
}

//-------------------------------------------------------------------------
vtkAscension3DGTracker::~vtkAscension3DGTracker() 
{
  if ( this->Recording )
  {
    this->StopTracking();
  }

  if ( this->AscensionRecordBuffer != NULL )
  {
    delete this->AscensionRecordBuffer; 
    this->AscensionRecordBuffer = NULL; 
  }

  if ( this->LocalTrackerBuffer != NULL )
  {
    this->LocalTrackerBuffer->Delete(); 
    this->LocalTrackerBuffer = NULL; 
  }
}

//-------------------------------------------------------------------------
void vtkAscension3DGTracker::PrintSelf( ostream& os, vtkIndent indent )
{
  vtkTracker::PrintSelf( os, indent );
}

//-------------------------------------------------------------------------
PlusStatus vtkAscension3DGTracker::Connect()
{
  LOG_TRACE( "vtkAscension3DGTracker::Connect" ); 

  if ( this->Probe()!=PLUS_SUCCESS )
  {
    LOG_ERROR("Connection probe failed");
    return PLUS_FAIL;
  }

  this->CheckReturnStatus( ATC::InitializeBIRDSystem() );

  ATC::SYSTEM_CONFIGURATION systemConfig;

  if (this->CheckReturnStatus( ATC::GetBIRDSystemConfiguration( &systemConfig ) ) != PLUS_SUCCESS)
  {
    LOG_ERROR("Connection initialization failed");
    return PLUS_FAIL;
  }  
  // Change to metric units.

  int metric = 1;
  if (this->CheckReturnStatus( ATC::SetSystemParameter( ATC::METRIC, &metric, sizeof( metric ) ) )!= PLUS_SUCCESS)
  {
    LOG_ERROR("Connection set to metric units failed");
    return PLUS_FAIL;
  }  

  // Go through all tools.

  int sensorID;
  ATC::DATA_FORMAT_TYPE formatType = ATC::DOUBLE_POSITION_ANGLES_MATRIX_QUATERNION_TIME_Q_BUTTON;

pSensor = new SENSOR_CONFIGURATION[systemConfig.numberSensors];

  for ( sensorID = 0; sensorID < systemConfig.numberSensors; ++ sensorID )
  {
    this->CheckReturnStatus( ATC::SetSensorParameter( sensorID, ATC::DATA_FORMAT, &formatType, sizeof( formatType ) ) );

    ATC::DEVICE_STATUS status;
    status = ATC::GetSensorStatus( sensorID );

    this->SensorSaturated.push_back( ( status & SATURATED ) ? true : false );
    this->SensorAttached.push_back( ( status & NOT_ATTACHED ) ? false : true );
    this->SensorInMotion.push_back( ( status & OUT_OF_MOTIONBOX ) ? false : true );
    this->TransmitterAttached = ( ( status & NO_TRANSMITTER_ATTACHED ) ? false : true );

  }


  this->NumberOfSensors = systemConfig.numberSensors; 

  if ( this->AscensionRecordBuffer == NULL )
  {
    this->AscensionRecordBuffer = new AscensionRecordType[this->NumberOfSensors ];
  }

  // Enable tools
  for ( int i = 0; i < this->GetNumberOfSensors(); i++ )
  {
    if ( this->SensorAttached[ i ] )
    {
      std::ostringstream portName; 
      portName << i; 
      vtkTrackerTool * tool = NULL; 
      if ( this->GetToolByPortName(portName.str().c_str(), tool) != PLUS_SUCCESS )
      {
        LOG_WARNING("Undefined connected tool found on port '" << portName.str() << "', disabled it until not defined in the config file: " << vtkPlusConfig::GetInstance()->GetDeviceSetConfigurationFileName() ); 
        this->SensorAttached[ i ] = false; 
      }
    }
  }

  // Check that all tools were connected that was defined in the configuration file
  for ( ToolIteratorType it = this->GetToolIteratorBegin(); it != this->GetToolIteratorEnd(); ++it)
  {
    std::stringstream convert(it->second->GetPortName());
    int port(-1); 
    if ( ! (convert >> port ) )
    {
      LOG_ERROR("Failed to convert tool '" << it->second->GetToolName() << "' port name '" << it->second->GetPortName() << "' to integer, please check config file: " << vtkPlusConfig::GetInstance()->GetDeviceSetConfigurationFileName() ); 
      return PLUS_FAIL; 
    }

    if ( !this->SensorAttached[ port ] )
    {
      LOG_WARNING("Sensor not attached for tool '" << it->second->GetToolName() << "' on port name '" << it->second->GetPortName() << "', please check config file: " << vtkPlusConfig::GetInstance()->GetDeviceSetConfigurationFileName() ); 
    }
  }
  return PLUS_SUCCESS; 
}

//-------------------------------------------------------------------------
PlusStatus vtkAscension3DGTracker::Disconnect()
{
  LOG_TRACE( "vtkAscension3DGTracker::Disconnect" ); 
  return this->StopTracking(); 
}

//-------------------------------------------------------------------------
PlusStatus vtkAscension3DGTracker::Probe()
{
  LOG_TRACE( "vtkAscension3DGTracker::Probe" ); 

  return PLUS_SUCCESS; 
} 

//-------------------------------------------------------------------------
PlusStatus vtkAscension3DGTracker::InternalStartTracking()
{
  LOG_TRACE( "vtkAscension3DGTracker::InternalStartTracking" ); 
  if ( this->Recording )
  {
    return PLUS_SUCCESS;
  }

  if ( ! this->InitAscension3DGTracker() )
  {
    LOG_ERROR( "Couldn't initialize vtkAscension3DGTracker" );
    return PLUS_FAIL;
  } 

  // Turn on the first attached transmitter.

  ATC::BOARD_CONFIGURATION boardConfig;
  this->CheckReturnStatus( GetBoardConfiguration( 0, &boardConfig ) );

  short selectID = TRANSMITTER_OFF;
  int i = 0;
  bool found = false;
  while( ( i < boardConfig.numberTransmitters ) && ( found == false ) )
  {
    ATC:: TRANSMITTER_CONFIGURATION transConfig;
    this->CheckReturnStatus( GetTransmitterConfiguration( i, &transConfig ) );
    if ( transConfig.attached )
    {
      selectID = i;
      found = true;
    }
    ++ i;
  }

  if (this->CheckReturnStatus( ATC::SetSystemParameter( ATC::SELECT_TRANSMITTER, &selectID, sizeof( selectID ) ) ) != PLUS_SUCCESS)
  {
    LOG_ERROR("Select transmitter failed");
    return PLUS_FAIL;
  }

  return PLUS_SUCCESS;
}

//-------------------------------------------------------------------------
PlusStatus vtkAscension3DGTracker::InternalStopTracking()
{
  LOG_TRACE( "vtkAscension3DGTracker::InternalStopTracking" ); 

  short selectID = TRANSMITTER_OFF;
  if (this->CheckReturnStatus( ATC::SetSystemParameter( ATC::SELECT_TRANSMITTER, &selectID, sizeof( selectID ) ) )
    != PLUS_SUCCESS)
  {
    LOG_ERROR("Select transmitter failed");
    return PLUS_FAIL;
  }

  return PLUS_SUCCESS;
}

//-------------------------------------------------------------------------
PlusStatus vtkAscension3DGTracker::InternalUpdate()
{
  LOG_TRACE( "vtkAscension3DGTracker::InternalUpdate" ); 

  if ( ! this->Recording )
  {
    LOG_ERROR("called Update() when not tracking" );
    return PLUS_FAIL;
  }

  ATC::SYSTEM_CONFIGURATION sysConfig;
  if (this->CheckReturnStatus( ATC::GetBIRDSystemConfiguration( &sysConfig ) )
    != PLUS_SUCCESS)
  {
    LOG_ERROR("Cannot get system configuration");
    return PLUS_FAIL;
  }

  if ( this->GetNumberOfSensors() != sysConfig.numberSensors )
  {
    LOG_ERROR( "Changing sensors while tracking is not supported. Reconnect necessary." );
    this->StopTracking();
    this->Disconnect();
    return PLUS_FAIL;
  }

  AscensionRecordType* record = static_cast<AscensionRecordType*>(this->AscensionRecordBuffer); 
  if ( record == NULL )
  {
    LOG_ERROR( "Ascension record buffer is invalid, reconnect necessary." );
    return PLUS_FAIL; 
  }

#ifdef PLUS_USE_Ascension3DGm
  // Request data from each sensor one-by-one. This method works well in some cases (with 3DGm systems?) when the
  // ALL_SENSORS data request times out.
  // Scan the sensors and request a record if the sensor is physically attached
  for(int sensorID=0;sensorID<sysConfig.numberSensors;sensorID++)
  {
    if((pSensor+sensorID)->attached)
    {
      // sensor attached so get record
      if (this->CheckReturnStatus( GetAsynchronousRecord(sensorID, record+sensorID, sizeof(*record)) ) != PLUS_SUCCESS)
      {
        LOG_ERROR("Cannot get synchronous record");
        return PLUS_FAIL;
      }
    }
  }
#else
  // Request data for all the sensors at once
  if (this->CheckReturnStatus( atc::  GetAsynchronousRecord( ALL_SENSORS,
    record, sysConfig.numberSensors * sizeof( AscensionRecordType ) ) ) != PLUS_SUCCESS)
  {
    LOG_ERROR("Cannot get synchronous record");
    return PLUS_FAIL;
  }
#endif

  // Set up reference matrix.

  bool saturated(false), attached(false), inMotionBox(false);
  bool transmitterRunning(false), transmitterAttached(false), globalError(false);

  ToolStatus toolStatus = TOOL_OK;
  const double unfilteredTimestamp = vtkAccurateTimer::GetSystemTime();
  int numberOfErrors(0); 

  for ( unsigned short sensorIndex = 0; sensorIndex < sysConfig.numberSensors; ++ sensorIndex )
  {
    if ( ! SensorAttached [ sensorIndex ] )
    {
      // Sensor disabled because it was not defined in the configuration file
      continue; 
    }

    ATC::DEVICE_STATUS status = ATC::GetSensorStatus( sensorIndex );

    saturated = status & SATURATED;
    attached = ! ( status & NOT_ATTACHED );
    inMotionBox = ! ( status & OUT_OF_MOTIONBOX );
    transmitterRunning = !( status & NO_TRANSMITTER_RUNNING );
    transmitterAttached = !( status & NO_TRANSMITTER_ATTACHED );
    globalError = status & GLOBAL_ERROR;

    vtkSmartPointer< vtkMatrix4x4 > mToolToTracker = vtkSmartPointer< vtkMatrix4x4 >::New();
    mToolToTracker->Identity();
    for ( int row = 0; row < 3; ++ row )
    {
      for ( int col = 0; col < 3; ++ col )
      {
        mToolToTracker->SetElement( row, col, record[ sensorIndex ].s[ row ][ col ] );
      }
    }
    
    mToolToTracker->Invert();

    mToolToTracker->SetElement( 0, 3, record[ sensorIndex ].x );
    mToolToTracker->SetElement( 1, 3, record[ sensorIndex ].y );
    mToolToTracker->SetElement( 2, 3, record[ sensorIndex ].z );

    if ( ! attached ) toolStatus = TOOL_MISSING;
    if ( ! inMotionBox ) toolStatus = TOOL_OUT_OF_VIEW;
    
    std::ostringstream toolPortName; 
    toolPortName << sensorIndex; 

    vtkTrackerTool * tool = NULL;
    if ( this->GetToolByPortName(toolPortName.str().c_str(), tool) != PLUS_SUCCESS )
    {
      LOG_ERROR("Unable to find tool on port: " << toolPortName.str() ); 
      numberOfErrors++; 
      continue; 
    }
          
    // Devices has no frame numbering, so just auto increment tool frame number
    unsigned long frameNumber = tool->GetFrameNumber() + 1 ; 
    this->ToolTimeStampedUpdate( tool->GetToolName(), mToolToTracker, toolStatus, frameNumber, unfilteredTimestamp);
  }

  return (numberOfErrors > 0 ? PLUS_FAIL : PLUS_SUCCESS);
}

//-------------------------------------------------------------------------
PlusStatus vtkAscension3DGTracker::InitAscension3DGTracker()
{
  LOG_TRACE( "vtkAscension3DGTracker::InitAscension3DGTracker" ); 
#ifdef PLUS_USE_Ascension3DGm  
  return PLUS_SUCCESS;
#else
  return this->Connect(); 
#endif  
}

//-------------------------------------------------------------------------
PlusStatus vtkAscension3DGTracker::CheckReturnStatus( int status )
{
  if( status != ATC::BIRD_ERROR_SUCCESS )
  {
    char buffer[ 512 ];
    ATC::GetErrorText( status, buffer, sizeof( buffer ), ATC::SIMPLE_MESSAGE );
    LOG_ERROR(buffer);
    return PLUS_FAIL;
  }
  return PLUS_SUCCESS;
}
