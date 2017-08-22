/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/

// Local includes
#include "PlusConfigure.h"
#include "PlusMath.h"
#include "PlusTrackedFrame.h"
#include "PlusVideoFrame.h"
#include "vtkPlusTrackedFrameList.h"
#include "vtkPlusTransverseProcessEnhancer.h"
#include "vtkPlusUsScanConvertCurvilinear.h"
#include "vtkPlusUsScanConvertLinear.h"

// VTK includes
#include <vtkImageAccumulate.h>
#include <vtkImageCast.h>
#include <vtkImageDilateErode3D.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkImageIslandRemoval2D.h>
#include <vtkImageSobel2D.h>
#include <vtkImageThreshold.h>
#include <vtkObjectFactory.h>

#include "vtkImageAlgorithm.h"

#include <cmath>


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlusTransverseProcessEnhancer);

//----------------------------------------------------------------------------
vtkPlusTransverseProcessEnhancer::vtkPlusTransverseProcessEnhancer()
  : ScanConverter(NULL),
  NumberOfScanLines(0),
  NumberOfSamplesPerScanLine(0),

  RadiusStartMm(0),
  RadiusStopMm(0),
  ThetaStartDeg(0),
  ThetaStopDeg(0),

  GaussianSmooth(NULL),
  EdgeDetector(NULL),
  ImageBinarizer(NULL),
  BinaryImageForMorphology(NULL),
  IslandRemover(NULL),
  ImageEroder(NULL),
  ImageDialator(NULL),

  ConversionImage(NULL),
  IslandAreaThreshold(-1),
  BoneOutlineDepthPx(3), //TODO: Change to mm (Note: this only changes the apperance/thickness of the 3D model. Different numbers do not change what is or isnt marked as bone.)
  BonePushBackPx(9), //TODO: change to mm

  LinesImage(NULL),
  ProcessedLinesImage(NULL),
  UnprocessedLinesImage(NULL)

{
  this->SetMmToPixelFanImage(0, 0, 0);
  this->SetMmToPixelLinesImage(0, 0, 0);

  this->GaussianSmooth = vtkSmartPointer<vtkImageGaussianSmooth>::New();    //Used to smooth the image
  this->EdgeDetector = vtkSmartPointer<vtkImageSobel2D>::New();             //Used to outline edges of the image
  this->ImageBinarizer = vtkSmartPointer<vtkImageThreshold>::New();         //Used to convert into a binary image
  this->BinaryImageForMorphology = vtkSmartPointer<vtkImageData>::New();    //The Binary image
  this->IslandRemover = vtkSmartPointer<vtkImageIslandRemoval2D>::New();    //Used to reomve islands (small isolated groups of pixels)
  this->ImageEroder = vtkSmartPointer<vtkImageDilateErode3D>::New();        //Used to Erode the image
  this->ImageDialator = vtkSmartPointer<vtkImageDilateErode3D>::New();      //Used to Dilate the image


  //Set the default parameters for the filters mentioned above

  this->SetDilationKernelSize(1, 1);
  this->SetErosionKernelSize(5, 5);
  this->SetGaussianStdDev(7.0);
  this->SetGaussianKernelSize(7.0);
  this->GaussianSmooth->SetDimensionality(2);

  this->ConversionImage = vtkSmartPointer<vtkImageData>::New();
  this->ConversionImage->SetExtent(0, 0, 0, 0, 0, 0);

  this->BinaryImageForMorphology->SetExtent(0, 0, 0, 0, 0, 0);
  this->ImageBinarizer->SetInValue(255);
  this->ImageBinarizer->SetOutValue(0);
  this->ImageBinarizer->ThresholdBetween(55, 255);

  this->IslandRemover->SetIslandValue(255);
  this->IslandRemover->SetReplaceValue(0);
  this->IslandRemover->SetAreaThreshold(0);

  this->ImageEroder->SetKernelSize(this->ErosionKernelSize[0], this->ErosionKernelSize[1], 1);
  this->ImageEroder->SetErodeValue(255);
  this->ImageEroder->SetDilateValue(0);

  this->ImageDialator->SetKernelSize(this->DilationKernelSize[0], this->DilationKernelSize[1], 1);
  this->ImageDialator->SetErodeValue(0);
  this->ImageDialator->SetDilateValue(255);

  this->LinesImage = vtkSmartPointer<vtkImageData>::New();
  this->ProcessedLinesImage = vtkSmartPointer<vtkImageData>::New();
  this->UnprocessedLinesImage = vtkSmartPointer<vtkImageData>::New();

  this->LinesImage->SetExtent(0, 0, 0, 0, 0, 0);
  this->ProcessedLinesImage->SetExtent(0, 0, 0, 0, 0, 0);
  this->UnprocessedLinesImage->SetExtent(0, 0, 0, 0, 0, 0);

  this->IntermediateImageMap.clear();
}

//----------------------------------------------------------------------------
vtkPlusTransverseProcessEnhancer::~vtkPlusTransverseProcessEnhancer()
{
  // Make sure contained smart pointers are deleted
  this->IntermediateImageMap.clear();
  this->IntermediatePostfixes.clear();
}

//----------------------------------------------------------------------------
void vtkPlusTransverseProcessEnhancer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusTransverseProcessEnhancer::ReadConfiguration(vtkSmartPointer<vtkXMLDataElement> processingElement)
{
  XML_VERIFY_ELEMENT(processingElement, this->GetTagName());

  //Read things in the ScanConversion tag
  vtkSmartPointer<vtkXMLDataElement> scanConversionElement = processingElement->FindNestedElementWithName("ScanConversion");
  if (scanConversionElement != NULL)
  {
    // Call scanline generator with appropriate scanconvert
    const char* transducerGeometry = scanConversionElement->GetAttribute("TransducerGeometry");
    if (transducerGeometry == NULL)
    {
      LOG_ERROR("Scan converter TransducerGeometry is undefined");
      return PLUS_FAIL;
    }
    else
    {
      LOG_INFO("Scan converter is defined.");
    }

    vtkSmartPointer<vtkPlusUsScanConvert> scanConverter;
    if (STRCASECMP(transducerGeometry, "CURVILINEAR") == 0)
    {
      this->ScanConverter = vtkSmartPointer<vtkPlusUsScanConvert>::Take(vtkPlusUsScanConvertCurvilinear::New());
    }
    else if (STRCASECMP(transducerGeometry, "LINEAR") == 0)
    {
      this->ScanConverter = vtkSmartPointer<vtkPlusUsScanConvert>::Take(vtkPlusUsScanConvertLinear::New());
    }
    else
    {
      LOG_ERROR("Invalid scan converter TransducerGeometry: " << transducerGeometry);
      return PLUS_FAIL;
    }
    this->ScanConverter->ReadConfiguration(scanConversionElement);

    XML_READ_SCALAR_ATTRIBUTE_OPTIONAL(int, RadiusStartMm, scanConversionElement);
    XML_READ_SCALAR_ATTRIBUTE_OPTIONAL(int, RadiusStopMm, scanConversionElement);
    XML_READ_SCALAR_ATTRIBUTE_OPTIONAL(int, ThetaStartDeg, scanConversionElement);
    XML_READ_SCALAR_ATTRIBUTE_OPTIONAL(int, ThetaStopDeg, scanConversionElement);
  }
  else
  {
    LOG_INFO("ScanConversion section not found in config file!");
  }

  // Read image processing options from configuration
  vtkXMLDataElement* imageProcessingOperations = processingElement->FindNestedElementWithName("ImageProcessingOperations");
  if (imageProcessingOperations != NULL)
  {

    //read tags relavent to the Gaussian filter
    vtkSmartPointer<vtkXMLDataElement> gaussianParameters = imageProcessingOperations->FindNestedElementWithName("GaussianSmoothing");
    if (gaussianParameters == NULL)
    {
      LOG_WARNING("Unable to locate GaussianSmoothing parameters element. Using default values.");
    }
    else
    {
      XML_READ_SCALAR_ATTRIBUTE_REQUIRED(double, GaussianStdDev, gaussianParameters);
      XML_READ_SCALAR_ATTRIBUTE_REQUIRED(double, GaussianKernelSize, gaussianParameters);
    }

    //read tags relavent to Island Removal
    vtkSmartPointer<vtkXMLDataElement> islandRemovalParameters = imageProcessingOperations->FindNestedElementWithName("IslandRemoval");
    if (islandRemovalParameters == NULL)
    {
      LOG_WARNING("Unable to locate IslandRemoval parameters element. Using default values.");
    }
    else
    {
      XML_READ_SCALAR_ATTRIBUTE_REQUIRED(int, IslandAreaThreshold, islandRemovalParameters);
    }

    //read tags relavent to Erosion
    vtkSmartPointer<vtkXMLDataElement> erosionParameters = imageProcessingOperations->FindNestedElementWithName("Erosion");
    if (erosionParameters == NULL)
    {
      LOG_WARNING("Unable to locate Erosion paramters element. Using default values.");
    }
    else
    {
      XML_READ_VECTOR_ATTRIBUTE_REQUIRED(int, 2, ErosionKernelSize, erosionParameters);
    }

    //read tags relavent to Dialation
    vtkSmartPointer<vtkXMLDataElement> dilationParameters = imageProcessingOperations->FindNestedElementWithName("Dilation");
    if (dilationParameters == NULL)
    {
      LOG_WARNING("Unable to locate Dilation parameters element. Using default values.");
    }
    else
    {
      XML_READ_VECTOR_ATTRIBUTE_REQUIRED(int, 2, DilationKernelSize, dilationParameters);
    }
  }
  else
  {
    //If this section in not in the xml file, use all filters with default values
    LOG_INFO("ImageProcessingOperations section not found in config file!");
    LOG_INFO("Enabling all filters and using default values.");
  }

  //Read tags relavent to scan lines
  XML_READ_SCALAR_ATTRIBUTE_REQUIRED(int, NumberOfScanLines, processingElement);
  XML_READ_SCALAR_ATTRIBUTE_REQUIRED(int, NumberOfSamplesPerScanLine, processingElement);

  int rfImageExtent[6] = { 0, this->NumberOfSamplesPerScanLine - 1, 0, this->NumberOfScanLines - 1, 0, 0 };
  this->ScanConverter->SetInputImageExtent(rfImageExtent);

  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
// Writes the perameters that were used to a config file
PlusStatus vtkPlusTransverseProcessEnhancer::WriteConfiguration(vtkSmartPointer<vtkXMLDataElement> processingElement)
{
  XML_VERIFY_ELEMENT(processingElement, this->GetTagName());

  //Write the parameters for filters to the scanner's properties to the output config file
  processingElement->SetAttribute("Type", this->GetProcessorTypeName());
  processingElement->SetIntAttribute("NumberOfScanLines", NumberOfScanLines);
  processingElement->SetIntAttribute("NumberOfSamplesPerScanLine", NumberOfSamplesPerScanLine);

  XML_FIND_NESTED_ELEMENT_CREATE_IF_MISSING(scanConversionElement, processingElement, "ScanConversion");
  this->ScanConverter->WriteConfiguration(scanConversionElement);
  scanConversionElement->SetDoubleAttribute("RadiusStartMm", this->RadiusStartMm);
  scanConversionElement->SetDoubleAttribute("RadiusStopMm", this->RadiusStopMm);
  scanConversionElement->SetIntAttribute("ThetaStartDeg", this->ThetaStartDeg);
  scanConversionElement->SetIntAttribute("ThetaStopDeg", this->ThetaStopDeg);

  //Write the parameters for filters to the output config file
  XML_FIND_NESTED_ELEMENT_CREATE_IF_MISSING(imageProcessingOperations, processingElement, "ImageProcessingOperations");

  XML_FIND_NESTED_ELEMENT_CREATE_IF_MISSING(gaussianParameters, imageProcessingOperations, "GaussianSmoothing");
  gaussianParameters->SetDoubleAttribute("GaussianStdDev", this->GaussianStdDev);
  gaussianParameters->SetDoubleAttribute("GaussianKernelSize", this->GaussianKernelSize);

  XML_FIND_NESTED_ELEMENT_CREATE_IF_MISSING(islandRemovalParameters, imageProcessingOperations, "IslandRemoval");
  islandRemovalParameters->SetIntAttribute("IslandAreaThreshold", IslandAreaThreshold);

  XML_FIND_NESTED_ELEMENT_CREATE_IF_MISSING(erosionParameters, imageProcessingOperations, "Erosion");
  erosionParameters->SetVectorAttribute("ErosionKernelSize", 2, this->ErosionKernelSize);

  XML_FIND_NESTED_ELEMENT_CREATE_IF_MISSING(dilationParameters, imageProcessingOperations, "Dilation");
  dilationParameters->SetVectorAttribute("DilationKernelSize", 2, this->DilationKernelSize);

  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusTransverseProcessEnhancer::ProcessImageExtents()
{
  // Allocate lines image.
  int* linesImageExtent = this->ScanConverter->GetInputImageExtent();

  LOG_DEBUG("Lines image extent: "
    << linesImageExtent[0] << ", " << linesImageExtent[1]
    << ", " << linesImageExtent[2] << ", " << linesImageExtent[3]
    << ", " << linesImageExtent[4] << ", " << linesImageExtent[5]);

  this->BinaryImageForMorphology->SetExtent(linesImageExtent);
  this->BinaryImageForMorphology->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

  this->LinesImage->SetExtent(linesImageExtent);
  this->LinesImage->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

  //Set up variables related to image extents
  int dims[3] = { 0, 0, 0 };
  this->LinesImage->GetDimensions(dims);

  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
// Fills the lines image by subsampling the input image along scanlines.
// Also computes pixel statistics.
void vtkPlusTransverseProcessEnhancer::FillLinesImage(vtkSmartPointer<vtkImageData> inputImageData)
{
  int* linesImageExtent = this->ScanConverter->GetInputImageExtent();
  int lineLengthPx = linesImageExtent[1] - linesImageExtent[0] + 1;
  int numScanLines = linesImageExtent[3] - linesImageExtent[2] + 1;

  // For calculating pixel intensity mean and variance. Algorithm taken from:
  // https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Online_algorithm

  double mean = 0.0;
  double sumSquareDiff = 0.0; //named M2 in online notes
  long pixelCount = 0;
  double currentValue = 0.0; //temporary value for each loop. //Named value in online notes
  double valueMeanDiff = 0.0; //Named delta in online notes

  double directionVectorX;
  double directionVectorY;
  int pixelCoordX;
  int pixelCoordY;

  int* inputExtent = inputImageData->GetExtent();
  for (int scanLine = 0; scanLine < numScanLines; ++scanLine)
  {
    double start[4] = { 0, 0, 0, 0 };
    double end[4] = { 0, 0, 0, 0 };
    ScanConverter->GetScanLineEndPoints(scanLine, start, end);

    directionVectorX = static_cast<double>(end[0] - start[0]) / (lineLengthPx - 1);
    directionVectorY = static_cast<double>(end[1] - start[1]) / (lineLengthPx - 1);
    for (int pointIndex = 0; pointIndex < lineLengthPx; ++pointIndex)
    {
      pixelCoordX = start[0] + directionVectorX * pointIndex;
      pixelCoordY = start[1] + directionVectorY * pointIndex;
      if ( pixelCoordX < inputExtent[0] || pixelCoordX > inputExtent[1]
        || pixelCoordY < inputExtent[2] || pixelCoordY > inputExtent[3] )
      {
        this->LinesImage->SetScalarComponentFromFloat(pointIndex, scanLine, 0, 0, 0);
        continue; // outside of the specified extent
      }
      currentValue = inputImageData->GetScalarComponentAsDouble(pixelCoordX, pixelCoordY, 0, 0);
      this->LinesImage->SetScalarComponentFromFloat(pointIndex, scanLine, 0, 0, currentValue);

      ++pixelCount;
      valueMeanDiff = currentValue - mean;
      mean = mean + valueMeanDiff / pixelCount;
      sumSquareDiff = sumSquareDiff + valueMeanDiff * (currentValue - mean);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPlusTransverseProcessEnhancer::VectorImageToUchar(vtkSmartPointer<vtkImageData> inputImage)
{
  unsigned char* vOutput = 0;
  unsigned char edgeDetectorOutput0;
  unsigned char edgeDetectorOutput1;
  float output = 0.0;     // Keep this in [0..255] instead [0..1] for possible future optimization.
  float output2 = 0.0;

  int dims[3] = { 0, 0, 0 };
  this->LinesImage->GetDimensions(dims);
  this->ConversionImage->SetExtent(this->LinesImage->GetExtent());
  this->ConversionImage->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
  for (int y = dims[1] - 1; y >= 0; --y)
  {
    // Initialize variables for a new scan line.

    for (int x = dims[0] - 1; x >= 0; --x)   // Go towards transducer
    {
      edgeDetectorOutput0 = static_cast<unsigned char>(inputImage->GetScalarComponentAsFloat(x, y, 0, 0));
      edgeDetectorOutput1 = static_cast<unsigned char>(inputImage->GetScalarComponentAsFloat(x, y, 0, 1));
      vOutput = static_cast<unsigned char*>(this->ConversionImage->GetScalarPointer(x, y, 0));
      output = (float)(edgeDetectorOutput0 + edgeDetectorOutput1) / (float)2;                                         // Not mathematically correct, but a quick approximation of sqrt(x^2 + y^2)

      *vOutput = (unsigned char)std::max(0, std::min(255, (int)output));
    }
  }
}

//----------------------------------------------------------------------------
/*
Takes a vtkSmartPointer<vtkImageData> as an argument and modifies it such that all images in a row
that have a bone shadow behind it are removed
*/
void vtkPlusTransverseProcessEnhancer::MarkShadowOutline(vtkSmartPointer<vtkImageData> inputImage)
{
  int dims[3] = { 0, 0, 0 };
  inputImage->GetDimensions(dims);

  int keepInfoCounter;
  bool foundBone;
  unsigned char* vOutput;

  int lastVistedValue = 0;

  //Setup variables for recording bone areas
  std::map<std::string, int> currentBoneArea;
  int boneAreaStart = dims[1] - 1;  //The y coordinate of where the bone outline starts
  int boneDepthSum = 0;             //The sum of the x coordinates of each pixel in the bone outline
  int boneMaxDepth = dims[0] - 1;   //The x coordinate of the right-most pixel in the bone outline
  int boneMinDepth = 0;             //The x coordinate of the left-most pixel in the bone outline
  int boneAreaDifferenceSlope = 3;  //If two pixels are seperated by this value or greater in the x coordinate, they are marked as seperate bones TODO: Magic Number, replace with parameter in mm

  for (int y = dims[1] - 1; y >= 0; --y)
  {

    //When an image is detected, keep up to this many pixles after it
    keepInfoCounter = this->BoneOutlineDepthPx + this->BonePushBackPx;
    foundBone = false;

    for (int x = dims[0] - 1; x >= 0; --x)
    {
      vOutput = static_cast<unsigned char*>(inputImage->GetScalarPointer(x, y, 0));

      //If an image is detected
      if (*vOutput != 0)
      {
        if (keepInfoCounter == 0 || keepInfoCounter > this->BoneOutlineDepthPx)
        {
          *vOutput = 0;
        }

        if (keepInfoCounter == this->BoneOutlineDepthPx + this->BonePushBackPx)
        {
          if (foundBone == false)
          {
            //found the first bone
            foundBone = true;

            //the two bone pixels are far enough appart, save them as being parts of different bone areas
            if (std::abs(x - lastVistedValue) >= boneAreaDifferenceSlope  && y != dims[1] - 1)
            {
              //check if the preveous area had any bone
              if (boneDepthSum != 0)
              {
                //Save info related to where the bone area
                currentBoneArea["depth"] = boneDepthSum / (boneAreaStart - y);                  // Store the outline's average x-coordinate
                currentBoneArea["xMax"] = boneMaxDepth;                                         // Store the outline's maximum x-coordinate (Used for efficiency)
                currentBoneArea["xMin"] = std::max(boneMinDepth - this->BoneOutlineDepthPx, 0); // Store the outline's minimum x-coordinate (Used for efficiency)
                currentBoneArea["yMax"] = boneAreaStart;                                        // Store the outline's maximum y-coordinate
                currentBoneArea["yMin"] = y + 1;                                                // Store the outline's minimum y-coordinate
                this->BoneAreasInfo.push_back(currentBoneArea);
                currentBoneArea.clear();
              }
              boneAreaStart = y;
              boneDepthSum = 0;
              boneMaxDepth = x;
              boneMinDepth = x;
            }
            else
            {
              if (x > boneMaxDepth)
              {
                boneMaxDepth = x;
              }
              if (x < boneMinDepth)
              {
                boneMinDepth = x;
              }
            }
            boneDepthSum += x;
            lastVistedValue = x;

          }
        }
      }
      if (foundBone == true && keepInfoCounter != 0)
      {
        if (keepInfoCounter <= this->BoneOutlineDepthPx && *vOutput == 0)
        {
          *vOutput = 255;
        }
        keepInfoCounter--;
      }
    }

    //if no bones were found on this row, but there was a bone before this, save it
    if (foundBone == false)
    {
      lastVistedValue = 0;
      if (boneDepthSum != 0)
      {
        //Save info related to where the bone area
        currentBoneArea["depth"] = boneDepthSum / (boneAreaStart - y);                  // Store the outline's average x-coordinate
        currentBoneArea["xMax"] = boneMaxDepth;                                         // Store the outline's maximum x-coordinate (Used for efficiency)
        currentBoneArea["xMin"] = std::max(boneMinDepth - this->BoneOutlineDepthPx, 0); // Store the outline's minimum x-coordinate (Used for efficiency)
        currentBoneArea["yMax"] = boneAreaStart;                                        // Store the outline's maximum y-coordinate
        currentBoneArea["yMin"] = y + 1;                                                // Store the outline's minimum y-coordinate
        this->BoneAreasInfo.push_back(currentBoneArea);
        boneDepthSum = 0;
        currentBoneArea.clear();
      }
      boneMaxDepth = dims[0] - 1;
      boneMinDepth = 0;

      boneAreaStart = y - 1;
    }
  }

  //save the last bone that goes off-screen
  if (boneDepthSum != 0)
  {
    //Save info related to where the bone area
    currentBoneArea["depth"] = boneDepthSum / (boneAreaStart + 1);                  // Store the outline's average x-coordinate
    currentBoneArea["xMax"] = boneMaxDepth;                                         // Store the outline's maximum x-coordinate (Used for efficiency)
    currentBoneArea["xMin"] = std::max(boneMinDepth - this->BoneOutlineDepthPx, 0); // Store the outline's minimum x-coordinate (Used for efficiency)
    currentBoneArea["yMax"] = boneAreaStart;                                        // Store the outline's maximum y-coordinate
    currentBoneArea["yMin"] = 0;                                                    // Store the outline's minimum y-coordinate
    this->BoneAreasInfo.push_back(currentBoneArea);
    currentBoneArea.clear();
  }
}

//----------------------------------------------------------------------------
/*
Takes a vtkSmartPointer<vtkImageData> with clearly defined possible bone segments as an
argument and modifies it so the bone areas that are too close to the camera's edge are removed.
*/
void vtkPlusTransverseProcessEnhancer::RemoveOffCameraBones(vtkSmartPointer<vtkImageData> inputImage)
{
  int dims[3] = { 0, 0, 0 };
  inputImage->GetDimensions(dims);

  unsigned char* vOutput = 0;

  int distanceVerticalBuffer = 10; //TODO: Magic Number, replace with parameter in mm
  int distanceHorizontalBuffer = 20; //TODO: Magic Number, replace with parameter in mm
  int boneMinSize = 10; //TODO: Magic Number, replace with parameter in mm
  std::vector<std::map<std::string, int>> boneAreas = this->BoneAreasInfo;

  int boneHalfLen;
  bool clearArea;
  bool foundBone;

  this->BoneAreasInfo.clear();

  for (int areaIndex = boneAreas.size() - 1; areaIndex >= 0; --areaIndex)
  {
    std::map<std::string, int> currentArea = boneAreas.at(areaIndex);

    clearArea = false;
    boneHalfLen = ((currentArea["yMax"] - currentArea["yMin"]) + 1)  / 2;

    //check if the bone is to close too the scan's edge
    if (currentArea["yMax"] + distanceVerticalBuffer >= dims[1] - 1 || currentArea["yMin"] - distanceVerticalBuffer <= 0)
    {
      clearArea = true;
    }
    //check if given the size, the bone is too close to the scan's edge
    else if (boneHalfLen + currentArea["yMax"] >= dims[1] - 1 || (currentArea["yMin"] - 1) - boneHalfLen <= 0)
    {
      clearArea = true;
    }
    //check if the bone is too close/far from the transducer 
    else if (currentArea["depth"] < distanceHorizontalBuffer || currentArea["depth"] > dims[0] - distanceHorizontalBuffer)
    {
      clearArea = true;
    }
    //check if the bone is to small
    else if (currentArea["yMax"] - currentArea["yMin"] <= boneMinSize)
    {
      clearArea = true;
    }

    //If it dosnt meet the criteria, remove the bones in this area
    if (clearArea == true)
    {
      //search through the area where the pixels are known to be
      for (int y = currentArea["yMax"]; y >= currentArea["yMin"]; --y)
      {
        int x = currentArea["xMax"] - this->BonePushBackPx;
        foundBone = false;
        while (x >= currentArea["xMin"] - this->BonePushBackPx && x >= 0 && foundBone == false)
        {
          vOutput = static_cast<unsigned char*>(inputImage->GetScalarPointer(x, y, 0));
          if (*vOutput != 0)
          {
            //remove all pixels in the outline
            *vOutput = 0;
            for (int removeBonex = std::max(0, x - (this->BoneOutlineDepthPx - 1)); removeBonex < x; ++removeBonex)
            {
              vOutput = static_cast<unsigned char*>(inputImage->GetScalarPointer(removeBonex, y, 0));
              *vOutput = 0;
            }

            foundBone = true;
          }
          x--;
        }
      }
    }
    else
    {
      this->BoneAreasInfo.push_back(currentArea);
    }
  }
}

//----------------------------------------------------------------------------
/*
Takes an unmodified vtkSmartPointer<vtkImageData> of an ultrasound as its first argument, and a more
enhanced version of said image, with clearly defined possible bone segments as the second argument.
This function modifies the second argument so as to remove any bone segments that have a higher
amount of bone potential in the areas next to it than there is within the areas themselves.
*/
void vtkPlusTransverseProcessEnhancer::CompareShadowAreas(vtkSmartPointer<vtkImageData> originalImage, vtkSmartPointer<vtkImageData> inputImage)
{
  int dims[3] = { 0, 0, 0 };
  inputImage->GetDimensions(dims);

  float vInput = 0;
  unsigned char* vOutput = 0;

  //Variables used for measuring the size and intensity sum for bone, above, and below areas
  int boneLen;
  int boneHalfLen;
  float boneArea;
  float aboveSum;
  float areaSum;
  float belowSum;

  float aboveAvgShadow; //Shadow intensity of the above area
  float areaAvgShadow;  //Shadow intensity of the area
  float belowAvgShadow; //Shadow intensity of the below area

  std::map<std::string, int> currentArea;
  std::vector<std::map<std::string, int>> boneAreas = this->BoneAreasInfo;

  bool foundBone;
  this->BoneAreasInfo.clear();

  for (int areaIndex = boneAreas.size() - 1; areaIndex >= 0; --areaIndex)
  {
    currentArea = boneAreas.at(areaIndex);

    aboveSum = 0;
    areaSum = 0;
    belowSum = 0;

    boneLen = (currentArea["yMax"] - currentArea["yMin"]) + 1;
    boneHalfLen = boneLen / 2;
    boneArea = boneLen * currentArea["depth"];

    //gather sum of shadow areas from above the area
    for (int y = currentArea["yMax"] + boneHalfLen; y > currentArea["yMax"]; --y)
    {
      for (int x = dims[0] - 1; x >= currentArea["depth"]; --x)
      {
        vInput = (originalImage->GetScalarComponentAsFloat(x, y, 0, 0));
        aboveSum += vInput;
      }
    }
    //gather sum of shadow areas from the area
    for (int y = currentArea["yMax"]; y >= currentArea["yMin"]; --y)
    {
      for (int x = dims[0] - 1; x >= currentArea["depth"]; --x)
      {
        vInput = (originalImage->GetScalarComponentAsFloat(x, y, 0, 0));
        areaSum += vInput;
      }
    }
    //gather sum of shadow areas from below the area
    for (int y = currentArea["yMin"] - boneHalfLen; y < currentArea["yMin"]; ++y)
    {
      for (int x = dims[0] - 1; x >= currentArea["depth"]; --x)
      {
        vInput = (originalImage->GetScalarComponentAsFloat(x, y, 0, 0));
        belowSum += vInput;
      }
    }

    //Calculate average shadow intensity
    aboveAvgShadow = aboveSum / (boneArea / 2);
    areaAvgShadow = areaSum / boneArea;
    belowAvgShadow = belowSum / (boneArea / 2);

    //If there is a higher amount of bones around it, remove the area
    if (aboveAvgShadow - areaAvgShadow <= areaAvgShadow / 2 || belowAvgShadow - areaAvgShadow <= areaAvgShadow / 2)
    {

      for (int y = currentArea["yMax"]; y >= currentArea["yMin"]; --y)
      {
        //search through the area where the pixels are known to be
        int x = currentArea["xMax"] - this->BonePushBackPx;
        foundBone = false;
        while (x >= currentArea["xMin"] - this->BonePushBackPx && x >= 0 && foundBone == false)
        {
          vOutput = static_cast<unsigned char*>(inputImage->GetScalarPointer(x, y, 0));
          if (*vOutput != 0)
          {
            //remove all pixels in the outline
            *vOutput = 0;

            for (int removeBonex = std::max(0, x - (this->BoneOutlineDepthPx - 1)); removeBonex < x; ++removeBonex)
            {
              vOutput = static_cast<unsigned char*>(inputImage->GetScalarPointer(removeBonex, y, 0));
              *vOutput = 0;
            }
            foundBone = true;
          }
          x--;
        }
      }
    }
    else
    {
      this->BoneAreasInfo.push_back(currentArea);
    }
  }
}

//----------------------------------------------------------------------------
//a way of threasholding based on the standard deviation of a row
void vtkPlusTransverseProcessEnhancer::ThresholdViaStdDeviation(vtkSmartPointer<vtkImageData> inputImage)
{
  int fatLayerToCut = 20; //TODO: Magic Number, replace with parameter in mm

  float vInput = 0;
  unsigned char* vOutput = 0;

  int dims[3] = { 0, 0, 0 };
  inputImage->GetDimensions(dims);

  int max;

  //values used to calculate the standard deviation
  int pixelSum;
  int squearSum;
  float pixelAverage;
  float meanDiffSum;
  float meanDiffAverage;
  float thresholdValue;

  for (int y = dims[1] - 1; y >= 0; --y)
  {
    max = 0;

    pixelSum = 0;
    squearSum = 0;
    pixelAverage = 0;

    //determine the average, sum, and max of the row
    for (int x = dims[0] - 1; x >= fatLayerToCut; --x)
    {
      vInput = inputImage->GetScalarComponentAsFloat(x, y, 0, 0);
      pixelSum += vInput;
      squearSum += vInput * vInput;

      if (vInput > max)
      {
        max = vInput;
      }
    }
    pixelAverage = pixelSum / (dims[0] - fatLayerToCut);

    //determine the standard deviation of the row
    meanDiffSum = squearSum + (dims[0] - fatLayerToCut) * pixelAverage * pixelAverage + (-2 * pixelAverage * pixelSum);
    meanDiffAverage = meanDiffSum / (dims[0] - fatLayerToCut);
    thresholdValue = max - 3 * pow(meanDiffAverage, 0.5);


    //if a pixel's value is too low, remove it
    if (pixelSum != 0)
    {
      for (int x = dims[0] - 1; x >= 0; --x)
      {
        vOutput = static_cast<unsigned char*>(inputImage->GetScalarPointer(x, y, 0));
        if (*vOutput < thresholdValue && *vOutput != 0)
        {
          *vOutput = 0;
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
// If a pixel in MaskImage is > 0, the corresponding pixel in InputImage will remain unchanged, otherwise it will be set to 0
void vtkPlusTransverseProcessEnhancer::ImageConjunction(vtkSmartPointer<vtkImageData> InputImage, vtkSmartPointer<vtkImageData> MaskImage)
{
  // Images must be of the same dimension, an should already be, I should check this though
  unsigned char* inputPixelPointer = 0;

  int dims[3] = { 0, 0, 0 };
  this->LinesImage->GetDimensions(dims);      // This will be the same as InputImage, as long as InputImage is converted to linesImage previously

  for (int y = dims[1] - 1; y >= 0; --y)
  {
    // Initialize variables for a new scan line.

    for (int x = dims[0] - 1; x >= 0; --x)   // Go towards transducer
    {
      if (static_cast<unsigned char>(MaskImage->GetScalarComponentAsFloat(x, y, 0, 0)) > 0)
      {
        //do nothing
      }
      else
      {
        inputPixelPointer = static_cast<unsigned char*>(InputImage->GetScalarPointer(x, y, 0));
        *inputPixelPointer = 0;
      }
    }
  }
}

//----------------------------------------------------------------------------
PlusStatus vtkPlusTransverseProcessEnhancer::ProcessFrame(PlusTrackedFrame* inputFrame, PlusTrackedFrame* outputFrame)
{

  if (this->FirstFrame == true)
  {
    //set up variables for future loops
    this->ProcessImageExtents();
    this->FirstFrame = false;
  }

  this->ScanConverter->GetOutputImageSpacing(this->MmToPixelFanImage);

  this->BoneAreasInfo.clear();

  PlusVideoFrame* inputImage = inputFrame->GetImageData();
  //an image used to transport output between filters
  vtkSmartPointer<vtkImageData> intermediateImage = vtkSmartPointer<vtkImageData>::New();

  if (this->ScanConverter.GetPointer() == NULL)
  {
    return PLUS_FAIL;
  }


  //Convert the image to a readable non-fan image
  this->ScanConverter->SetInputData(inputImage->GetImage());
  // Generate lines image.
  this->AddIntermediateFromFilter("_01Lines_1PreFillLines", this->ScanConverter);
  this->FillLinesImage(inputImage->GetImage());
  this->AddIntermediateImage("_01Lines_2FilterEnd", this->LinesImage);
  intermediateImage->DeepCopy(this->LinesImage);

  int dimsFan[3] = { 0, 0, 0 };
  inputImage->GetImage()->GetDimensions(dimsFan);
  int dimsLines[3] = { 0, 0, 0 };
  this->LinesImage->GetDimensions(dimsLines);

  this->SetMmToPixelLinesImage(this->MmToPixelFanImage[0] * ((double)dimsLines[0] / (double)dimsFan[1]), this->MmToPixelFanImage[1] * ((double)dimsLines[1] / (double)dimsFan[0]), this->MmToPixelFanImage[2] * ((double)dimsLines[2] / (double)dimsFan[2]));


  //Save this image so that it can be used for comparason with the output image
  vtkSmartPointer<vtkImageData> originalImage = vtkSmartPointer<vtkImageData>::New();
  originalImage->DeepCopy(intermediateImage);

  //Threashold the image based on the standard deviation of a pixel's columns
  this->ThresholdViaStdDeviation(intermediateImage);
  this->AddIntermediateImage("_02Threshold_1FilterEnd", intermediateImage);

  //Use gaussian smoothing
  this->GaussianSmooth->SetInputData(intermediateImage);
  this->AddIntermediateFromFilter("_03Gaussian_1FilterEnd", this->GaussianSmooth);

  //Edge detection
  this->EdgeDetector->SetInputConnection(this->GaussianSmooth->GetOutputPort());
  this->EdgeDetector->Update();
  this->VectorImageToUchar(this->EdgeDetector->GetOutput());
  this->AddIntermediateImage("_04EdgeDetector_1FilterEnd", this->ConversionImage);

  // Since we perform morphological operations, we must binarize the image
  this->ImageBinarizer->SetInputData(this->ConversionImage);
  this->AddIntermediateFromFilter("_05BinaryImageForMorphology_1FilterEnd", this->ImageBinarizer);

  //Remove small clusters of pixels
  this->IslandRemover->SetInputConnection(this->ImageBinarizer->GetOutputPort());
  this->IslandRemover->Update();
  this->AddIntermediateImage("_06Island_1FilterEnd", this->IslandRemover->GetOutput());

  //Erode the image
  this->ImageEroder->SetKernelSize(this->ErosionKernelSize[0], this->ErosionKernelSize[1], 1);
  this->ImageEroder->SetInputConnection(this->IslandRemover->GetOutputPort());
  this->AddIntermediateFromFilter("_07Erosion_1FilterEnd", this->ImageEroder);

  //Dilate the image
  this->ImageDialator->SetKernelSize(this->DilationKernelSize[0], this->DilationKernelSize[1], 1);
  this->ImageDialator->SetInputConnection(this->ImageEroder->GetOutputPort());
  this->ImageDialator->Update();
  this->BinaryImageForMorphology->DeepCopy(this->ImageDialator->GetOutput());
  this->AddIntermediateImage("_08Dilation_1FilterEnd", this->BinaryImageForMorphology);

  //Detect each possible bone area, then subject it to various tests to confirm if it is valid
  this->MarkShadowOutline(this->BinaryImageForMorphology);
  this->AddIntermediateImage("_09PostFilters_1ShadowOutline", this->BinaryImageForMorphology);
  this->RemoveOffCameraBones(this->BinaryImageForMorphology);
  this->AddIntermediateImage("_09PostFilters_2PostRemoveOffCamera", this->BinaryImageForMorphology);
  this->CompareShadowAreas(originalImage, this->BinaryImageForMorphology);
  this->AddIntermediateImage("_09PostFilters_3PostCompareShadowAreas", this->BinaryImageForMorphology);

  //Reconvert the image to greyscale
  // Currently, inputImage is the output of the edge detector, not original pixels
  this->UnprocessedLinesImage->DeepCopy(this->GaussianSmooth->GetOutput());
  this->ImageConjunction(this->UnprocessedLinesImage, this->BinaryImageForMorphology);
  this->AddIntermediateImage("_10ReconvertBinaryToGreyscale_1FilterEnd", this->UnprocessedLinesImage);
  intermediateImage->DeepCopy(this->UnprocessedLinesImage);

  //Setup so that the image can be converted into a fan-image
  this->ProcessedLinesImage->DeepCopy(intermediateImage);
  PlusVideoFrame processedVideoFrame;
  processedVideoFrame.DeepCopyFrom(this->ProcessedLinesImage);
  PlusTrackedFrame* processedTrackedFrame = inputFrame;
  processedTrackedFrame->SetImageData(processedVideoFrame);

  //Setup so that the image can be converted into a fan-image
  PlusVideoFrame* outputImage = outputFrame->GetImageData();
  this->ScanConverter->SetInputData(this->ProcessedLinesImage);
  this->ScanConverter->SetOutput(intermediateImage);
  this->ScanConverter->Update();

  outputImage->DeepCopyFrom(intermediateImage);

  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
/*
Finds and saves all intermediate images that have been recorded.
Saves the images by calling this->SaveIntermediateResultToFile()
Returns PLUS_FAIL if this->SaveIntermediateResultToFile() encounters an error occured during this
process, returns PLUS_SUCCESS otherwise.
*/
PlusStatus vtkPlusTransverseProcessEnhancer::SaveAllIntermediateResultsToFile()
{
  for (int postfixIndex = this->IntermediatePostfixes.size() - 1; postfixIndex >= 0; postfixIndex -= 1)
  {
    if (this->SaveIntermediateResultToFile(this->IntermediatePostfixes.at(postfixIndex)) == PLUS_FAIL)
    {
      return PLUS_FAIL;
    }
  }
  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
/*
Takes a postfix as an argument and saves the intermediate image associated with that postfix
Returns PLUS_FAIL if an error occured during this process, returns PLUS_SUCCESS otherwise
*/
PlusStatus vtkPlusTransverseProcessEnhancer::SaveIntermediateResultToFile(char* fileNamePostfix)
{
  std::map<char*, vtkSmartPointer<vtkPlusTrackedFrameList> >::iterator indexIterator = this->IntermediateImageMap.find(fileNamePostfix);
  if (indexIterator != this->IntermediateImageMap.end())
  {

    //Try to save the intermediate image
    if (this->IntermediateImageMap[fileNamePostfix]->SaveToSequenceMetafile(IntermediateImageFileName + "_Plus" + std::string(fileNamePostfix) + ".mha", US_IMG_ORIENT_MF, false) == PLUS_FAIL)
    {
      LOG_ERROR("An issue occured when trying to save the intermediate image with the postfix: " << fileNamePostfix);
      return PLUS_FAIL;
    }
    else
    {
      LOG_INFO("Sucessfully wrote the intermediate image with the postfix: " << fileNamePostfix);
    }
  }

  return PLUS_SUCCESS;
}

//----------------------------------------------------------------------------
void vtkPlusTransverseProcessEnhancer::AddIntermediateImage(char* fileNamePostfix, vtkSmartPointer<vtkImageData> image)
{
  if (fileNamePostfix == "")
  {
    LOG_WARNING("The empty string was given as an intermediate image file postfix.");
  }

  if (this->SaveIntermediateResults) //TODO: Do this check when calling this method, not inside it
  {

    // See if the intermediate image should be created
    std::map<char*, vtkSmartPointer<vtkPlusTrackedFrameList> >::iterator indexIterator = this->IntermediateImageMap.find(fileNamePostfix);
    if (indexIterator != this->IntermediateImageMap.end()){}
    else
    {
      // Create if not found
      this->IntermediateImageMap[fileNamePostfix] = vtkPlusTrackedFrameList::New();

      this->IntermediatePostfixes.push_back(fileNamePostfix);
    }

    //Add the current frame to its vtkPlusTrackedFrameList
    PlusVideoFrame linesVideoFrame;
    linesVideoFrame.DeepCopyFrom(image);
    PlusTrackedFrame linesTrackedFrame;
    linesTrackedFrame.SetImageData(linesVideoFrame);
    this->IntermediateImageMap[fileNamePostfix]->AddTrackedFrame(&linesTrackedFrame);
  }
}

//----------------------------------------------------------------------------
//Given a vtk filter, get the image that would display at that point and save it
void vtkPlusTransverseProcessEnhancer::AddIntermediateFromFilter(char* fileNamePostfix, vtkImageAlgorithm* imageFilter)
{
  if (fileNamePostfix == "")
  {
    LOG_WARNING("The empty string was given as an intermediate image file postfix.");
  }

  if (this->SaveIntermediateResults) //TODO: Do this check when calling this method, not inside it
  {
    vtkSmartPointer<vtkImageData> tempOutputImage = vtkSmartPointer<vtkImageData>::New();
    imageFilter->SetOutput(tempOutputImage);
    imageFilter->Update();

    //this->AddIntermediateImage(fileNamePostfix, tempOutputImage);
  }
}

//----------------------------------------------------------------------------
void vtkPlusTransverseProcessEnhancer::SetGaussianStdDev(double gaussianStdDev)
{
  this->GaussianStdDev = gaussianStdDev;
  this->GaussianSmooth->SetStandardDeviation(gaussianStdDev);
}

//----------------------------------------------------------------------------
void vtkPlusTransverseProcessEnhancer::SetGaussianKernelSize(double gaussianKernelSize)
{
  this->GaussianKernelSize = gaussianKernelSize;
  this->GaussianSmooth->SetRadiusFactor(gaussianKernelSize);
}

//----------------------------------------------------------------------------
void vtkPlusTransverseProcessEnhancer::SetIslandAreaThreshold(int islandAreaThreshold)
{
  this->IslandAreaThreshold = islandAreaThreshold;
  if (islandAreaThreshold < 0)
  {
    this->IslandRemover->SetAreaThreshold(0);
  }
  else
  {
    this->IslandRemover->SetAreaThreshold(islandAreaThreshold);
  }
}