#ifndef __FabricSpliceDialogs_H_
#define __FabricSpliceDialogs_H_

#include <xsi_customproperty.h>
#include <xsi_string.h>

XSI::CustomProperty editorPropGet();
XSI::CustomProperty editorPropGetEnsureExists(bool clearErrors = false);
void editorSetObjectIDFromSelection();
void showSpliceEcitor(unsigned int objectID);
const char * getSourceCodeForOperator(const char * graphName, const char * opName);
void updateSpliceEditorGrids(XSI::CustomProperty & prop);

class AddPortDialog
{
public:
  AddPortDialog(int portPurpose, XSI::CString defaultPortMode = L"IN");
  ~AddPortDialog();
  bool getDataType(XSI::CString & portType, XSI::CString & extension);
  bool show(XSI::CString title);
  XSI::CustomProperty getProp() { return _prop; }
private:
  XSI::CustomProperty _prop;
};

class AddOperatorDialog
{
public:
  AddOperatorDialog();
  ~AddOperatorDialog();
  bool show(XSI::CString title);
  XSI::CustomProperty getProp() { return _prop; }
private:
  XSI::CustomProperty _prop;
};

class ImportSpliceDialog
{
public:
  ImportSpliceDialog(XSI::CString fileName);
  ~ImportSpliceDialog();
  bool show(XSI::CString title);
  XSI::CustomProperty getProp() { return _prop; }
private:
  XSI::CustomProperty _prop;
};

class LicenseDialog
{
public:
  LicenseDialog();
  ~LicenseDialog();
  bool show();
  XSI::CustomProperty getProp() { return _prop; }
private:
  XSI::CustomProperty _prop;
};

#endif
