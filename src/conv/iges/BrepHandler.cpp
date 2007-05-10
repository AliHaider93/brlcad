#include "n_iges.hpp"

#include <iostream>
using namespace std;

namespace brlcad {

  BrepHandler::BrepHandler() {}

  BrepHandler::~BrepHandler() {}

  void
  BrepHandler::extract(IGES* iges, const DirectoryEntry* de) {
    _iges = iges;
    extractBrep(de);
  }

  void 
  BrepHandler::extractBrep(const DirectoryEntry* de) {
    debug("########################### E X T R A C T   B R E P");
    ParameterData params;
    _iges->getParameter(de->paramData(), params);
    
    Pointer shell = params.getPointer(1);
    extractShell(_iges->getDirectoryEntry(shell), false, params.getLogical(2));
    int numVoids = params.getInteger(3);
    
    if (numVoids <= 0) return;    
    
    int index = 4;
    for (int i = 0; i < numVoids; i++) {
      shell = params.getPointer(index);      
      extractShell(_iges->getDirectoryEntry(shell),
		   true,
		   params.getLogical(index+1));
      index += 2;
    }
  }
  
  void
  BrepHandler::extractShell(const DirectoryEntry* de, bool isVoid, bool orientWithFace) {
    debug("########################### E X T R A C T   S H E L L");
    ParameterData params;
    _iges->getParameter(de->paramData(), params);
    
    handleShell(isVoid, orientWithFace);

    int numFaces = params.getInteger(1);
    for (int i = 1; i <= numFaces; i++) {
      Pointer facePtr = params.getPointer(i*2);
      bool orientWithSurface = params.getLogical(i*2+1);
      DirectoryEntry* faceDE = _iges->getDirectoryEntry(facePtr);
      extractFace(faceDE, orientWithSurface);
    }
  }

  void 
  BrepHandler::extractFace(const DirectoryEntry* de, bool orientWithSurface) {
    // spec says the surface can be:
    //   parametric spline surface
    //   ruled surface
    //   surface of revolution
    //   tabulated cylinder
    //   rational b-spline surface
    //   offset surface
    //   plane surface
    //   rccyl surface
    //   rccone surface
    //   spherical surface
    //   toroidal surface

    debug("########################### E X T R A C T   F A C E"); 
    ParameterData params;
    _iges->getParameter(de->paramData(), params);
        
    Pointer surfaceDE = params.getPointer(1);
    int surf = extractSurface(_iges->getDirectoryEntry(surfaceDE));

    int face = handleFace(orientWithSurface, surf);

    int numLoops = params.getInteger(2);
    bool isOuter = params.getLogical(3);    
    for (int i = 4; (i-4) < numLoops; i++) {
      Pointer loopDE = params.getPointer(i);
      extractLoop(_iges->getDirectoryEntry(loopDE), isOuter, face);
      isOuter = false;
    }
  }

  int
  BrepHandler::extractSurface(const DirectoryEntry* de) {
    debug("########################## E X T R A C T   S U R F A C E");
    ParameterData params;
    _iges->getParameter(de->paramData(), params);
    // determine the surface type to extract
    switch (de->type()) {
    case ParametricSplineSurface:      
      break;
    case RuledSurface:
      break;
    case SurfaceOfRevolution:
      break;
    case TabulatedCylinder:
      break;
    case RationalBSplineSurface: {
      // possible to do optimization of form type???
      // see spec
      const int ui = params.getInteger(1);
      const int vi = params.getInteger(2);
      int u_degree = params.getInteger(3);
      int v_degree = params.getInteger(4);
      bool u_closed = params.getInteger(5)() == 1;
      bool v_closed = params.getInteger(6)() == 1;
      bool rational = params.getInteger(7)() == 0;
      bool u_periodic = params.getInteger(8)() == 1;
      bool v_periodic = params.getInteger(9)() == 1;
      
      int n1 = 1+ui-u_degree;
      int n2 = 1+vi-v_degree;
      
      const int u_num_knots = n1 + 2 * u_degree;
      const int v_num_knots = n2 + 2 * v_degree;
      const int num_weights = (1+ui)*(1+vi);
      
      // read the u knots
      int i = 10; // first u knot
      double* u_knots = new double[u_num_knots];
      for (int _i = 0; _i < u_num_knots; _i++) {
	u_knots[_i] = params.getReal(i);
	i++;
      }
      i = 11 + u_num_knots; // first v knot
      double* v_knots = new double[v_num_knots];
      for (int _i = 0; _i < v_num_knots; _i++) {
	v_knots[_i] = params.getReal(i);
	i++;
      }
      
      // read the weights (w)
      i = 11 + u_num_knots + v_num_knots;
      double* weights = new double[num_weights];
      for (int _i = 0; _i < num_weights; _i++) {
	weights[_i] = params.getReal(i);
	i++;
      }
      
      // read the control points
      i = 12 + u_num_knots + v_num_knots + num_weights;
      double* ctl_points = new double[CP_SIZE(ui+1, vi+1, 3)];
      const int numu = ui+1;
      const int numv = vi+1;
      for (int _v = 0; _v < numv; _v++) {
	for (int _u = 0; _u < numu; _u++) {
	  ctl_points[CPI(_u,_v,0)] = params.getReal(i);
	  ctl_points[CPI(_u,_v,1)] = params.getReal(i+1);
	  ctl_points[CPI(_u,_v,2)] = params.getReal(i+2);
	  i += 3;
	}
      }
      
      // read the domain intervals
      double umin = params.getReal(i);
      double umax = params.getReal(i+1);
      double vmin = params.getReal(i+2);
      double vmax = params.getReal(i+3);

      int controls[] = {ui+1,vi+1};
      int degrees[] = {u_degree,v_degree};
      int index = handleRationalBSplineSurface( controls,
						degrees,
						u_closed,
						v_closed,
						rational,
						u_periodic,
						v_periodic,
						u_num_knots,
						v_num_knots,
						u_knots,
						v_knots,
						weights,
						ctl_points);
      delete [] ctl_points;
      delete [] weights;
      delete [] v_knots;
      delete [] u_knots;      
    } break;
    case OffsetSurface:
      break;
    case PlaneSurface:
      break;
    case RightCircularCylindricalSurface:
      break;
    case RightCircularConicalSurface:
      break;
    case SphericalSurface:
      break;
    case ToroidalSurface:
      break;
    }
    return 0;
  }

  class PSpaceCurve {
  public:
    PSpaceCurve(IGES* _iges, BrepHandler* _brep, Logical& iso, Pointer& c) 
    {
      DirectoryEntry* curveDE = _iges->getDirectoryEntry(c);
      ParameterData param;
      _iges->getParameter(curveDE->paramData(), param);
    }
    PSpaceCurve(const PSpaceCurve& ps) 
      : isIso(ps.isIso), curveIndex(ps.curveIndex) {}

    Logical isIso;
    int curveIndex;
  };
      

  class EdgeUse {
  public:
    EdgeUse(IGES* _iges, BrepHandler* _brep, Pointer& edgeList, int index)
    { 
      debug("########################## E X T R A C T   E D G E  U S E");
      DirectoryEntry* edgeListDE = _iges->getDirectoryEntry(edgeList);
      ParameterData params;
      _iges->getParameter(edgeListDE->paramData(), params);
      int paramIndex = (index-1)*5 + 1;
      Pointer msCurvePtr = params.getPointer(paramIndex);
      initVertexList = params.getPointer(paramIndex+1);
      initVertexIndex = params.getInteger(paramIndex+2);
      termVertexList = params.getPointer(paramIndex+3);
      termVertexIndex = params.getInteger(paramIndex+4);
      
      // extract the model space curves
      mCurveIndex = _brep->extractCurve(_iges->getDirectoryEntry(msCurvePtr), false);
    }
    EdgeUse(const EdgeUse& eu) 
      : mCurveIndex(eu.mCurveIndex),
	pCurveIndices(eu.pCurveIndices),
	initVertexList(eu.initVertexList), 
	initVertexIndex(eu.initVertexIndex), 
	termVertexList(eu.termVertexList), 
	termVertexIndex(eu.termVertexIndex) {}
    
    int mCurveIndex; // model space curve???
    list<PSpaceCurve> pCurveIndices; // parameter space curves
    Pointer initVertexList;
    Integer initVertexIndex;
    Pointer termVertexList;
    Integer termVertexIndex;    
  };

  void 
  BrepHandler::extractLoop(const DirectoryEntry* de, bool isOuter, int face) {
    debug("########################## E X T R A C T   L O O P");
    ParameterData params;
    _iges->getParameter(de->paramData(), params);

    int loop = handleLoop(isOuter, face);
    int numberOfEdges = params.getInteger(1);

    int i = 2; // extract the edge uses!
    for (int _i = 0; _i < numberOfEdges; _i++) {
      bool isVertex = (1 == params.getInteger(i)) ? true : false;
      Pointer edgePtr = params.getPointer(i+1);
      int index = params.getInteger(i+2);
      // need to get the edge list, and extract the edge info
      EdgeUse eu(_iges, this, edgePtr, index);
      bool orientWithCurve = params.getLogical(i+3);
      int numCurves = params.getInteger(i+4);
      debug("Num param-space curves in " << string((isOuter)?"outer":"inner") << " loop: " << numCurves);
      int j = i+5;
      for (int _j = 0; _j < numCurves; _j++) {
	// handle the param-space curves, which are generally not included in MSBO
	Logical iso = params.getLogical(j);
	Pointer ptr = params.getPointer(j+1);
	eu.pCurveIndices.push_back(PSpaceCurve(_iges,
					       this, 
					       iso,
					       ptr));
	j += 2;
      }
      i = j;
    }
  }

  void 
  BrepHandler::extractEdge(const DirectoryEntry* de) {
    
  }
  
  void 
  BrepHandler::extractVertex(const DirectoryEntry* de) {

  }

  int 
  BrepHandler::extractCurve(const DirectoryEntry* de, bool isISO) {
    // spec says the curve may be:
    //   100 Circular Arc 
    //   102 Composite Curve 
    //   104 Conic Arc 
    //   106/11 2D Path 
    //   106/12 3D Path 
    //   106/63 Simple Closed Planar Curve 
    //   110 Line 
    //   112 Parametric Spline Curve 
    //   126 Rational B-Spline Curve 
    //   130 Offset Curve 

    
  }

}