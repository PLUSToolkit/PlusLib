#include "PlusConfigure.h"
#include "vtkVideoBuffer.h"
#include "vtkObjectFactory.h"
#include "vtkVideoFrame2.h"
#include "vtkDoubleArray.h"
#include "vtkUnsignedLongLongArray.h"
#include "vtkCriticalSection.h"
#include "vtkImageData.h"

//----------------------------------------------------------------------------
//						VideoBufferItem
//----------------------------------------------------------------------------
VideoBufferItem::VideoBufferItem()
{
	this->Frame = NULL; 
}

//----------------------------------------------------------------------------
VideoBufferItem::~VideoBufferItem()
{
	if (this->Frame != NULL)
	{
		this->Frame->Delete();
		this->Frame = NULL; 
	}
}

//----------------------------------------------------------------------------
VideoBufferItem::VideoBufferItem(const VideoBufferItem &videoItem)
{
    this->Frame = NULL; 
    *this = videoItem; 
}

//----------------------------------------------------------------------------
VideoBufferItem& VideoBufferItem::operator=(VideoBufferItem const&videoItem)
{
    // Handle self-assignment
    if (this == &videoItem)
    {
        return *this;
    }

    if ( videoItem.GetFrame() != NULL && 
        this->SetFrameFormat(videoItem.GetFrame()) != PLUS_SUCCESS ) 
    {
        LOG_ERROR("Failed to set frame format for video buffer item!"); 
    }

    if ( this->Frame != NULL ) 
    {
        this->Frame->DeepCopy( videoItem.GetFrame() ); 
    }

	this->FilteredTimeStamp = videoItem.FilteredTimeStamp; 
	this->UnfilteredTimeStamp = videoItem.UnfilteredTimeStamp; 
	this->Index = videoItem.Index; 
	this->Uid = videoItem.Uid; 
    
    return *this;
}

//----------------------------------------------------------------------------
PlusStatus VideoBufferItem::DeepCopy(VideoBufferItem* videoItem)
{
	if ( videoItem == NULL )
	{
		LOG_ERROR("Failed to deep copy video buffer item - buffer item NULL!"); 
		return PLUS_FAIL; 
	}

	if ( videoItem->GetFrame() != NULL )
	{
        if ( this->SetFrameFormat(videoItem->GetFrame()) != PLUS_SUCCESS ) 
        {
            LOG_ERROR("Failed to allocate frame for video buffer item!"); 
            return PLUS_FAIL; 
        }

        this->Frame->DeepCopy( videoItem->GetFrame() ); 
	}

	this->FilteredTimeStamp = videoItem->FilteredTimeStamp; 
	this->UnfilteredTimeStamp = videoItem->UnfilteredTimeStamp; 
	this->Index = videoItem->Index; 
	this->Uid = videoItem->Uid; 

	return PLUS_SUCCESS; 
}

//----------------------------------------------------------------------------
PlusStatus VideoBufferItem::SetFrameFormat(vtkVideoFrame2* frameFormat)
{
	if ( frameFormat == NULL ) 
	{
		LOG_ERROR("Unable to set frame format - frame format is NULL!"); 
		return PLUS_FAIL; 
	}

    if ( this->Frame != NULL && this->Frame->CheckFrameFormat(frameFormat) )
    {
        // Farme format is the same, no need to change anything 
        return PLUS_SUCCESS; 
    }

	if ( this->Frame != NULL )
	{
		this->Frame->Delete(); 
		this->Frame = NULL; 
	}

	// Create frame object and allocate space
	this->Frame = frameFormat->MakeObject(); 
	if (!this->Frame->Allocate())
    {
        LOG_ERROR("Failed to allocate memory for video frame!"); 
        return PLUS_FAIL; 
    }

	return PLUS_SUCCESS; 
}



//----------------------------------------------------------------------------
PlusStatus VideoBufferItem::SetFrame(vtkVideoFrame2* frame)
{
	if ( frame == NULL )
	{
		LOG_ERROR( "Failed to add NULL frame to video buffer!"); 
		return PLUS_FAIL;
	}

	if ( this->Frame == NULL ) 
	{
		this->Frame = vtkVideoFrame2::New(); 
	}
	this->Frame->DeepCopy( frame ); 

	return PLUS_SUCCESS; 
}

//----------------------------------------------------------------------------
PlusStatus VideoBufferItem::SetFrame(vtkImageData* frame)
{
	const int* frameExtent = frame->GetExtent(); 
	const int frameSize[3] = {(frameExtent[1] - frameExtent[0] + 1), (frameExtent[3] - frameExtent[2] + 1), (frameExtent[5] - frameExtent[4] + 1) }; 
	const int numberOfBytes = frame->GetScalarSize(); 
	return this->SetFrame( reinterpret_cast<unsigned char*>(frame->GetScalarPointer()), frameSize, numberOfBytes, 0); 
}

//----------------------------------------------------------------------------
PlusStatus VideoBufferItem::SetFrame(unsigned char *imageDataPtr, 
									 const int frameSizeInPx[3],
									 const int numberOfBitsPerPixel, 
									 const int	numberOfBytesToSkip )
{
	if ( imageDataPtr == NULL )
	{
		LOG_ERROR( "Failed to add NULL frame to video buffer!"); 
		return PLUS_FAIL;
	}

	if ( this->Frame == NULL )
	{
		LOG_ERROR( "Unable to add frame to video buffer - need to allocate frame first!"); 
		return PLUS_FAIL; 
	}

	// Get frame buffer frame information
	unsigned char *frameBufferPtr = reinterpret_cast<unsigned char *>(this->Frame->GetVoidPointer(0));
	int frameBufferExtent[6] = {0};
	this->Frame->GetFrameExtent(frameBufferExtent);
	const int frameBufferSize[3] = {(frameBufferExtent[1] - frameBufferExtent[0]+1), (frameBufferExtent[3] - frameBufferExtent[2]+1), (frameBufferExtent[5] - frameBufferExtent[4]+1)}; 
	const int frameBufferBitsPerPixel = this->Frame->GetBitsPerPixel();
	const int outBytesPerRow = (frameBufferSize[0] * frameBufferBitsPerPixel + 7)/8;
	const int inBytesPerRow = (frameSizeInPx[0] * numberOfBitsPerPixel + 7)/8;
	int rows = frameBufferExtent[3] - frameBufferExtent[2]+1;

	if ( frameSizeInPx[0] != frameBufferSize[0] 
	|| frameSizeInPx[1] != frameBufferSize[1] 
	|| frameSizeInPx[2] != frameBufferSize[2] )
	{
		LOG_WARNING("Input frame size is different from buffer frame size (input: " << frameSizeInPx[0] << "x" << frameSizeInPx[1] << "x" << frameSizeInPx[2] 
		<< ",   buffer: " << frameBufferSize[0] << "x" << frameBufferSize[1] << "x" << frameBufferSize[2] << ")!"); 
	}

	// Skip the numberOfBytesToSkip bytes, e.g. header size
	imageDataPtr += numberOfBytesToSkip; 

	// copy data to the local vtk frame buffer
	if (outBytesPerRow == inBytesPerRow)
	{
		memcpy(frameBufferPtr,imageDataPtr,inBytesPerRow*rows);
	}
	else
	{
		for (int r = 0; r < rows; ++r)
		{
			memcpy(frameBufferPtr,imageDataPtr,outBytesPerRow);
			frameBufferPtr += outBytesPerRow;
			imageDataPtr += inBytesPerRow;
		}
	}

	return PLUS_SUCCESS; 
}

//----------------------------------------------------------------------------
//						vtkVideoBuffer
//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkVideoBuffer, "$Revision: 1.0 $");
vtkStandardNewMacro(vtkVideoBuffer);

//----------------------------------------------------------------------------
vtkVideoBuffer::vtkVideoBuffer()
{
	this->FrameFormat = vtkVideoFrame2::New();
	this->VideoBuffer = vtkTimestampedCircularBuffer<VideoBufferItem>::New(); 
}

//----------------------------------------------------------------------------
vtkVideoBuffer::~vtkVideoBuffer()
{ 
    if ( this->FrameFormat != NULL )
    {
	    this->FrameFormat->Delete();
        this->FrameFormat = NULL; 
    }

    if ( this->VideoBuffer != NULL )
    {
        this->VideoBuffer->Delete(); 
        this->VideoBuffer = NULL; 
    }
}

//----------------------------------------------------------------------------
void vtkVideoBuffer::PrintSelf(ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os,indent);

	if (this->FrameFormat)
	{
		os << indent << "FrameFormat:\n";
		this->FrameFormat->PrintSelf(os, indent.GetNextIndent());
	}
}

//----------------------------------------------------------------------------
void vtkVideoBuffer::UpdateBufferFrameFormats()
{
	this->VideoBuffer->Lock(); 
	for ( int i = 0; i < this->VideoBuffer->GetBufferSize(); ++i )
	{
		this->VideoBuffer->GetBufferItem(i)->SetFrameFormat(this->FrameFormat); 
	}
	this->VideoBuffer->Unlock(); 
}

//----------------------------------------------------------------------------
void vtkVideoBuffer::SetLocalTimeOffset(double offset)
{
	this->VideoBuffer->SetLocalTimeOffset(offset); 
}

//----------------------------------------------------------------------------
double vtkVideoBuffer::GetLocalTimeOffset()
{
	return this->VideoBuffer->GetLocalTimeOffset(); 
}

//----------------------------------------------------------------------------
int vtkVideoBuffer::GetBufferSize()
{
	return this->VideoBuffer->GetBufferSize(); 
}

//----------------------------------------------------------------------------
PlusStatus vtkVideoBuffer::SetBufferSize(int bufsize)
{
	return this->VideoBuffer->SetBufferSize(bufsize); 
}

//----------------------------------------------------------------------------
// Sets the frame format.  If the format is different from the old format,
// deletes all of the old frames and fills the buffer with new frames of the
// new format
void vtkVideoBuffer::SetFrameFormat(vtkVideoFrame2 *format)
{
	// if the new format matches the old format, we don't need to do anything
	if ( this->CheckFrameFormat(format) )
	{
		return; 
	}

	// set the new frame format
	if (this->FrameFormat)
	{
		this->FrameFormat->Delete();
	}
	if (format)
	{
		format->Register(this);
		this->FrameFormat = format;
	}
	else
	{
		this->FrameFormat = vtkVideoFrame2::New();
	}

	this->Modified();

	// replace all the frames with ones in the new format
	this->UpdateBufferFrameFormats(); 
}

//----------------------------------------------------------------------------
bool vtkVideoBuffer::CheckFrameFormat( const int frameSizeInPx[3], const int numberOfBitsPerPixel )
{
	// don't add a frame if it doesn't match the buffer frame format
	int frameFormatSize[3]={0};
	this->FrameFormat->GetFrameSize(frameFormatSize);
	if (frameSizeInPx[0] != frameFormatSize[0] ||
		frameSizeInPx[1] != frameFormatSize[1] ||
		frameSizeInPx[2] != frameFormatSize[2] ||
		numberOfBitsPerPixel != this->FrameFormat->GetBitsPerPixel()
		)
	{
		LOG_DEBUG("Frame format and buffer frame format does not match!"); 
		return false;
	}

	return true;
}

//----------------------------------------------------------------------------
bool vtkVideoBuffer::CheckFrameFormat( vtkVideoFrame2* frame )
{
    return this->FrameFormat->CheckFrameFormat(frame); 
}

//----------------------------------------------------------------------------
PlusStatus vtkVideoBuffer::AddItem(unsigned char *imageDataPtr, 
							  const int frameSizeInPx[3],
							  const int numberOfBitsPerPixel, 
							  const int	numberOfBytesToSkip, 
							  const double unfilteredTimestamp, 
							  const double filteredTimestamp, 
							  const long frameNumber)
{

	if ( imageDataPtr == NULL )
	{
		LOG_ERROR( "vtkVideoBuffer: Unable to add NULL frame to video buffer!"); 
		return PLUS_FAIL; 
	}

	if ( !this->CheckFrameFormat(frameSizeInPx, numberOfBitsPerPixel) )
	{
		LOG_ERROR( "vtkVideoBuffer: Unable to add frame to video buffer - frame format doesn't match!"); 
		return PLUS_FAIL; 
	}

	int bufferIndex(0); 
	BufferItemUidType itemUid; 
    this->VideoBuffer->Lock(); 
	if ( this->VideoBuffer->PrepareForNewFrame(filteredTimestamp, itemUid, bufferIndex) != PLUS_SUCCESS )
	{
        this->VideoBuffer->Unlock(); 
        // Just a debug message, because we want to avoid unnecessary warning messages if the timestamp is the same as last one
		LOG_DEBUG( "vtkVideoBuffer: Failed to prepare for adding new frame to video buffer!"); 
		return PLUS_FAIL; 
	}

	// get the pointer to the correct location in the frame buffer, where this data needs to be copied
	VideoBufferItem* newObjectInBuffer = this->VideoBuffer->GetBufferItem(bufferIndex); 
	if ( newObjectInBuffer == NULL )
	{
        this->VideoBuffer->Unlock(); 
		LOG_ERROR( "vtkVideoBuffer: Failed to get pointer to video buffer object from the video buffer for the new frame!"); 
		return PLUS_FAIL; 
	}

	PlusStatus status = newObjectInBuffer->SetFrame(imageDataPtr, frameSizeInPx, numberOfBitsPerPixel, numberOfBytesToSkip); 
	newObjectInBuffer->SetFilteredTimestamp(filteredTimestamp); 
	newObjectInBuffer->SetUnfilteredTimestamp(unfilteredTimestamp); 
	newObjectInBuffer->SetIndex(frameNumber); 
	newObjectInBuffer->SetUid(itemUid); 
    this->VideoBuffer->Unlock(); 

	return status; 
}

//----------------------------------------------------------------------------
PlusStatus vtkVideoBuffer::AddItem(vtkImageData* frame, const double unfilteredTimestamp, const double filteredTimestamp, const long frameNumber)
{
	const int* frameExtent = frame->GetExtent(); 
	const int frameSize[3] = {(frameExtent[1] - frameExtent[0] + 1), (frameExtent[3] - frameExtent[2] + 1), (frameExtent[5] - frameExtent[4] + 1) }; 
	const int numberOfBits = frame->GetScalarSize() * 8; 
	return this->AddItem( reinterpret_cast<unsigned char*>(frame->GetScalarPointer()), frameSize, numberOfBits, 0, unfilteredTimestamp, filteredTimestamp, frameNumber); 
}

//----------------------------------------------------------------------------
ItemStatus vtkVideoBuffer::GetLatestTimeStamp( double& latestTimestamp )
{
    return this->VideoBuffer->GetLatestTimeStamp(latestTimestamp); 
}

//----------------------------------------------------------------------------
ItemStatus vtkVideoBuffer::GetVideoBufferItem(const BufferItemUidType uid, VideoBufferItem* bufferItem)
{
	if ( bufferItem == NULL )
	{
		LOG_ERROR("Unable to copy video buffer item into a NULL video buffer item!"); 
		return ITEM_UNKNOWN_ERROR; 
	}

	ItemStatus status = this->VideoBuffer->GetFrameStatus(uid); 
	if ( status != ITEM_OK )
	{
		if (  status == ITEM_NOT_AVAILABLE_ANYMORE )
		{
			LOG_WARNING("Failed to get video buffer item: video item not available anymore"); 
		}
		else if (  status == ITEM_NOT_AVAILABLE_YET )
		{
			LOG_WARNING("Failed to get video buffer item: video item not available yet"); 
		}
		else
		{
			LOG_WARNING("Failed to get video buffer item!"); 
		}
		return status; 
	}

	if ( bufferItem->GetFrame() == NULL || !this->CheckFrameFormat(bufferItem->GetFrame()) )
	{
		bufferItem->SetFrame( this->FrameFormat ); 
	}

	VideoBufferItem* videoItem = this->VideoBuffer->GetBufferItem(uid); 

	if ( bufferItem->DeepCopy(videoItem) != PLUS_SUCCESS )
	{
		status = ITEM_UNKNOWN_ERROR; 
	}
	
	return status; 
}

//----------------------------------------------------------------------------
ItemStatus vtkVideoBuffer::GetVideoBufferItemFromTime( const double time, VideoBufferItem* bufferItem)
{
	BufferItemUidType uid(0); 
	ItemStatus status = this->VideoBuffer->GetItemUidFromTime(time, uid); 
	if ( status != ITEM_OK )
	{
		if (  status == ITEM_NOT_AVAILABLE_ANYMORE )
		{
			LOG_WARNING("Failed to get video buffer item from time: video item not available anymore"); 
		}
		else if (  status == ITEM_NOT_AVAILABLE_YET )
		{
			LOG_WARNING("Failed to get video buffer item from time: video item not available yet"); 
			
		}
		else
		{
			LOG_WARNING("Failed to get video buffer item from time!"); 
		}
		return status; 
	}

	return this->GetVideoBufferItem(uid, bufferItem);  
}

//----------------------------------------------------------------------------
void vtkVideoBuffer::DeepCopy(vtkVideoBuffer* buffer)
{
	this->VideoBuffer->DeepCopy( buffer->VideoBuffer ); 

	this->SetFrameFormat(buffer->GetFrameFormat()); 
	this->SetBufferSize(buffer->GetBufferSize()); 
	this->UpdateBufferFrameFormats(); 
}

//----------------------------------------------------------------------------
void vtkVideoBuffer::Clear()
{
	this->VideoBuffer->Clear(); 
}


