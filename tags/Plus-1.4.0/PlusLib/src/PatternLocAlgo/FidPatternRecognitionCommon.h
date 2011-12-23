/*=Plus=header=begin======================================================
  Program: Plus
  Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
  See License.txt for details.
=========================================================Plus=header=end*/

#ifndef _FIDUCIAL_ALGORITHM_COMMON_H
#define _FIDUCIAL_ALGORITHM_COMMON_H

#include <vector>

typedef unsigned char PixelType;

//-----------------------------------------------------------------------------
/*!
  \class Dot
  \brief This class defines a single dot made obtained from the segmentation part of the algorithm.
         It contains the X and Y coordinate of the dot as well as its intensity. Also contains an operator 
         to check if dots are equal (both X and Y are the same).
  \ingroup PlusLibPatternRecognition
*/
class Dot
{
	public:
    /*! Compare the intensity of 2 dots */
		static bool IntensityLessThan( Dot &dot1, Dot &dot2 );

    /*! Compare the position of 2 dots */
    static bool PositionLessThan( std::vector<Dot>::iterator b1, std::vector<Dot>::iterator b2 );

    /*! Set the x coordinate of the dot */
		void	SetX(float value) { m_X = value; };
    
    /*! Get the x coordinate of the dot */
		float	GetX() { return m_X; };
    
    /*! Set the y coordinate of the dot */
		void	SetY(float value) { m_Y = value; };
    
    /*! Get the y coordinate of the dot */
		float	GetY() { return m_Y; };

    /*! Set the intensity of the dot */
		void	SetDotIntensity(float value) { m_DotIntensity = value; };

    /*! Get the intensity of the dot */
		float	GetDotIntensity() { return m_DotIntensity; };

    /*! Compare two dots, coordinate-wise */
    bool operator== (const Dot& data) const { return (m_X == data.m_X && m_Y == data.m_Y) ; }

	protected:
		float	m_X;
		float	m_Y;
		float	m_DotIntensity;
};

//-----------------------------------------------------------------------------
/*!
  \class Line
  \brief This class defines a single line made from a vector of dots obtained from the segmentation part of the algorithm.
         It contains the list of indexes of the dots, the length of the line, the origin and the endpoint as well as the
         line intensity and its direction vector.
  \ingroup PlusLibPatternRecognition
*/
class Line
{
	public:
    /*! Compare the intensity of 2 lines */
		static bool lessThan( Line &line1, Line &line2 );

    /*! Compare 2 lines to know if they are the same */
		static bool compareLines( Line line1, Line line2 );

    /*! Compute the angle in radiants between the line and the positive x-axis, value between -Pi and + Pi */
    static float ComputeAngle(Line &line1);

     /*! Compute the half space angle in radiants between the line and the positive x-axis, value between -Pi/2 and + Pi/2 */
    static float ComputeHalfSpaceAngle(Line &line);

    /*! Set the point of line
        \param aIndex is the index in the dots vector fo the line
        \param aValue is the index of the dot in the general vector of dots
    */
    void	SetPoint(int aIndex, int aValue) { m_Points[aIndex] = aValue; };

    /*! Get the point of the line that has index aIndex */
    int	GetPoint(int aIndex) const{ return m_Points[aIndex]; };

    /*! Get the vector of dots making the line */
    std::vector<int>*	GetPoints() { return &m_Points; };

    /*! Set the intensity of the line, which is the sum of its dots intensity */
		void	SetIntensity(float value) { m_Intensity = value; };

    /*! Get the intensity of the line, which is the sum of its dots intensity */
		float	GetIntensity() { return m_Intensity; };

    /*! Set the length of the line */
		void SetLength(float value) { m_Length = value; };
    
    /*! Get the length of the line */
		float	GetLength() { return m_Length; };

    /*! Set the direction vector that defines the line */
    void	SetDirectionVector(int aIndex, float aValue) { m_DirectionVector[aIndex] = aValue; };

    /*! Get the direction vector that defines the line */
    float	GetDirectionVector(int aIndex) const{ return m_DirectionVector[aIndex]; };

    /*! Set the Origin of the line. It is the index of the origin in the m_DotsVector */
    void  SetOrigin(int value) { m_Origin = value; };

    /*! Get the Origin of the line. It is the index of the origin in the m_DotsVector */
    int   GetOrigin() { return m_Origin; };

    /*! Set the other end point of the line. It is the index of the end poin in the m_DotsVector */
    void  SetEndPoint(int value) { m_EndPoint = value; };

    /*! Get the other end point of the line. It is the index of the end poin in the m_DotsVector */
    int   GetEndPoint() { return m_EndPoint; };
    
    
	protected:
		std::vector<int>	m_Points; // indices of points that make up the line
		float				m_Intensity;
		float				m_Length;
    float       m_DirectionVector[2];
    int         m_Origin;//index of the line origin
    int         m_EndPoint;//Index of the endpoint of the line
    

};

//-----------------------------------------------------------------------------
/*!
  \class Wire
  \brief This structure defines a single fiducial wire by its name and its endpoint (front and back)
  \ingroup PlusLibPatternRecognition
*/
struct Wire
{
  std::string Name;
	double EndPointFront[3];
	double EndPointBack[3];
};

//-----------------------------------------------------------------------------
/*!
  \class Pattern
  \brief This class stores the different Patterns defined in the configuration file. It contains the wires
         defintion, the distance from the line origin of each expected "dot" and the tolerances on these
         distances.
  \ingroup PlusLibPatternRecognition
*/
struct Pattern
{
  std::vector<Wire> Wires;
  std::vector<float> DistanceToOriginMm;//These distances are in mm.
  std::vector<float> DistanceToOriginToleranceMm;//These tolerances are in mm.

  //unused iterator to iterate only through one type of Patterns.
/*
  template<class T> class iterator
  {
    public:
      iterator(std::vector<Pattern>* ptr) : m_Pointer(ptr) {}
      iterator operator++() 
      {
        do
        {
          T* t = dynamic_cast<T*>(m_Pointer);
          if(t)//belongs to the type T
          {
            iterator i = *this;
            m_Pointer++;
            return i;
          }
          m_Pointer++;
        }while(m_Pointer != NULL);
        return *this;
      }
    protected:
      std::vector<Pattern> * m_Pointer;
  };
*/
};

//-----------------------------------------------------------------------------
/*!
  \class NWire
  \brief The struct NWire is a child from Pattern and has two more features that are the intersections of the NWires
         between lines 1 and 2, and, 2 and 3.
  \ingroup PlusLibPatternRecognition
*/
struct NWire : public Pattern
{
  double  IntersectPosW12[3];
  double  IntersectPosW32[3];
};

//-----------------------------------------------------------------------------
/*!
  \class CoplanarParallelWires
  \brief The struct CoplanarparallelWires is a child from Pattern and represents an undefinite number of coplanar 
         parallel fiducial wires.
  \ingroup PlusLibPatternRecognition
*/
struct CoplanarParallelWires : public Pattern
{
};

//-----------------------------------------------------------------------------
/*!
  \class PatternRecognitionResult
  \brief This class stores the Pattern Recognition algorithm results.
  \ingroup PlusLibPatternRecognition
*/
class PatternRecognitionResult
{
	public:
		PatternRecognitionResult();
		virtual ~PatternRecognitionResult();

    /*! Clear the classe attributes once they are not needed anymore */
		void Clear();

    /*! Set the m_DotsFound to true if the algorithm found the corresponding dots, to false otherwise */
		void	SetDotsFound(bool value) { m_DotsFound = value; };

    /*! Get m_DotsFound */
		bool	GetDotsFound() { return m_DotsFound; };

    /*! Set the coordinates of the found dots */
		void	SetFoundDotsCoordinateValue(std::vector< std::vector<double> > value) { m_FoundDotsCoordinateValue = value; };

    /*! Get the coordinates of the found dots */
		std::vector< std::vector<double> >	GetFoundDotsCoordinateValue() { return m_FoundDotsCoordinateValue; };

    /*! Set the cumulate intensity of all dots in the pattern */
		void	SetIntensity(float value) { m_Intensity = value; };

    /*! Get the cumulate intensity of all dots in the pattern */
		float	GetIntensity() { return m_Intensity; };

    /*! Set the number of candidate points that have been found */
		void	SetNumDots(double value) { m_NumDots = value; };

    /*! Get the number of candidate points that have been found */
		double GetNumDots() { return m_NumDots; };

    /*! Set the dots that are considered candidates */
		void SetCandidateFidValues(std::vector<Dot> value) { m_CandidateFidValues = value; };

    /*! Get the dots that are considered candidates */
		std::vector<Dot>	GetCandidateFidValues() { return m_CandidateFidValues; };

	protected:
		/*! True if the dots are found, false otherwise. */
		bool m_DotsFound;

		/*! X and Y values of found dots. */
		std::vector< std::vector<double> >	m_FoundDotsCoordinateValue;

		/*! The combined intensity of the dots. This is the sum of the pixel
		  values after the morphological operations, with the pixel values on the
		  range 0-1.  A good intensity score is over 100. A bad one (but still
		  valid) is below 25. */
		float m_Intensity;

    /*! number of possibel fiducial points */
		double m_NumDots; 

    /*! pointer to the fiducial candidates coordinates */
		std::vector<Dot>	m_CandidateFidValues; 
};

//-----------------------------------------------------------------------------

#endif //_FIDUCIAL_ALGORITHM_COMMON_H
