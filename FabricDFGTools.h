#ifndef __FabricDFGTools_H_
#define __FabricDFGTools_H_

#include <xsi_x3dobject.h>
#include <xsi_ref.h>
#include <xsi_status.h>
#include <xsi_value.h>
#include <xsi_string.h>

#include <vector>

struct _portMapping;
struct _polymesh;

class dfgTools
{
 public:

  // opens a file browser for "*.canvas" files.
  // params:  isSave        true: save file browser, false: load file browser.
  //          out_filepath  complete filepath or "" on abort or error.
  // return: true on success, false on abort or error.
  static bool FileBrowserJSON(bool isSave, XSI::CString &out_filepath);
  
  // for a given X3DObject: fills the CRef array out_refs with all refs
  // at in_opName custom operators that belong to the X3DObject.
  // return value: the amount of custom operators that were found.
  static int GetRefsAtOps(XSI::X3DObject &in_obj, XSI::CString &in_opName, XSI::CRefArray &out_refs);
  
  // for a given custom operator: allocates and fills out_pmap based on the op's DFG, ports, etc.
  // returns: true on success, otherwise false plus an error description in out_err.
  static bool GetOperatorPortMapping(XSI::CRef &in_op, std::vector<_portMapping> &out_pmap, XSI::CString &out_err);
  
  // returns the matching siClassID for a given DFG resolved data type.
  static XSI::siClassID GetSiClassIdFromResolvedDataType(const XSI::CString &resDataType);
  
  // gets the description of a siCLassID.
  static XSI::CString &GetSiClassIdDescription(const XSI::siClassID in_siClassID, XSI::CString &out_description);

  // checks the existence of a file.
  // params:  filePath    path and filename
  // returns: true if file exists.
  static bool FileExists(const char *filePath);

  // gets the geometry from the input X3DObject.
  // params:  in_x3DObj             ref at X3DObject.
  //          in_currFrame          current frame.
  //          in_useGlobalSRT       consider object's global transformation.
  //          out_vertexPositions   ref at CDoubleArray that will contain the vertex positions.
  //          out_polyVIndices      ref at CLongArray that will contain the polygon vertex indices.
  //          out_polyVCount        ref at CLongArray that will contain the polygon vertex indices count.
  //          out_numNodes          ref at LONG that will contain the amount of nodes.
  //          inout_useVertMotions  ref at bool. Input: if true then get vertex motions (if available). Output: true if vertex motions were found and put in out_vertMotions.
  //          out_vertMotions       ref at CFloatArray that will contain the vertex motions.
  //          noWarnMotions         if true: don't abort if some data could not be found/gathered.
  //          inout_useNodeNormals  ref at bool. Input: if true then get node user normals (if available). Output: true if user normals were found and put in out_nodeNormals.
  //          in_useNormals         if this is true and if no user normals are available then use "normal" normals (this is only used if inout_useNodeNormals == true).
  //          out_nodeNormals       ref at CFloatArray that will contain the node normals.
  //          noWarnNormals         if true: don't abort if some data could not be found/gathered.
  //          inout_useNodeUVWs     ref at bool. Input: if true then get node UVWs (if available). Output: true if node UVWs were found and put in out_nodeUVWs.
  //          out_nodeUVWs          ref at CFloatArray that will contain the node UVWs.
  //          noWarnUVWs            if true: don't abort if some data could not be found/gathered.
  //          inout_useNodeColors   ref at bool. Input: if true then get node colors (if available). Output: true if node colors were found and put in out_nodeColors.
  //          out_nodeColors        ref at CFloatArray that will contain the node Colors.
  //          noWarnColors          if true: don't abort if some data could not be found/gathered.
  //          in_nameMotions        L"" or name of the motion data to use.
  //          in_nameNormals        L"" or name of the normal data to use.
  //          in_nameUVWs           L"" or name of the UVW data to use.
  //          in_nameColors         L"" or name of the color data to use.
  //          errmsg                ref at CString that will contain an error description if this functions returns false.
  //          wrnmsg                ref at CString that will contain a warning if this functions returns true, or L"" if no warning.
  // returns: true on success else false (with an error description in errmsg).
  // note: this function was taken "as is" from the class MZDXSI which is part of the main Mootzoid c++ lib (see mootzoid_XSI.h and mootzoid_XSI.cpp).
  static bool GetGeometryFromX3DObject(const XSI::X3DObject &in_x3DObj, double in_currFrame, bool in_useGlobalSRT, XSI::CDoubleArray &out_vertexPositions, XSI::CLongArray &out_polyVIndices, XSI::CLongArray &out_polyVCount, LONG &out_numNodes, bool &inout_useVertMotions, XSI::CFloatArray &out_vertMotions, bool noWarnMotions, bool &inout_useNodeNormals, bool in_useNormals, XSI::CFloatArray &out_nodeNormals, bool noWarnNormals, bool &inout_useNodeUVWs, XSI::CFloatArray &out_nodeUVWs, bool noWarnUVWs, bool &inout_useNodeColors, XSI::CFloatArray &out_nodeColors, bool noWarnColors, XSI::CString &in_nameMotions, XSI::CString &in_nameNormals, XSI::CString &in_nameUVWs, XSI::CString &in_nameColors, XSI::CString &errmsg, XSI::CString &wrnmsg);

  // gets the geometry from the input X3DObject and stores it in out_polymesh.
  // params:  in_x3DObj             ref at X3DObject.
  //          in_currFrame          current frame.
  //          out_polymesh          result.
  //          out_errmsg            ref at CString that will contain an error description if this functions returns false.
  //          out_wrnmsg            ref at CString that will contain a warning if this functions returns true, or L"" if no warning.
  // returns: true on success else false (with an error description in out_errmsg).
  static bool GetGeometryFromX3DObject(const XSI::X3DObject &in_x3DObj, double in_currFrame, _polymesh &out_polymesh, XSI::CString &out_errmsg, XSI::CString &out_wrnmsg);

  // returns the current amount of undo levels of the Softimage preferences or -1 if an error occurred.
  static LONG GetUndoLevels(void);

  // sets the amount of undo levels of the Softimage preferences and returns true on success.
  static bool SetUndoLevels(LONG undoLevels);

  // clears the Softimage undo/redo history and returns true on success.
  static bool ClearUndoHistory(void);

  // returns true if the operator/object belongs to a reference model.
  static bool belongsToRefModel(const XSI::CustomOperator &op);
  static bool belongsToRefModel(const XSI::X3DObject &obj);
};

#endif
