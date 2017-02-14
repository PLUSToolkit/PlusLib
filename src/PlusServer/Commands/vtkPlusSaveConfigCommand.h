/*=Plus=header=begin======================================================
  Program: Plus
  Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
  See License.txt for details.
=========================================================Plus=header=end*/

#ifndef __vtkPlusSaveConfigCommand_h
#define __vtkPlusSaveConfigCommand_h

#include "vtkPlusServerExport.h"

#include "vtkPlusCommand.h"

class vtkMatrix4x4;

/*!
  \class vtkPlusSaveConfigCommand
  \brief This command forces the data collector to save the config file
  \ingroup PlusLibPlusServer
 */
class vtkPlusServerExport vtkPlusSaveConfigCommand : public vtkPlusCommand
{
public:

  static vtkPlusSaveConfigCommand* New();
  vtkTypeMacro(vtkPlusSaveConfigCommand, vtkPlusCommand);
  virtual void PrintSelf(ostream& os, vtkIndent indent);
  virtual vtkPlusCommand* Clone() { return New(); }

  /*! Executes the command  */
  virtual PlusStatus Execute();

  /*! Read command parameters from XML */
  virtual PlusStatus ReadConfiguration(vtkXMLDataElement* aConfig);

  /*! Write command parameters to XML */
  virtual PlusStatus WriteConfiguration(vtkXMLDataElement* aConfig);

  /*! Get all the command names that this class can execute */
  virtual void GetCommandNames(std::list<std::string>& cmdNames);

  /*! Gets the description for the specified command name. */
  virtual std::string GetDescription(const std::string& commandName);

  virtual const std::string& GetFilename() const;
  virtual void SetFilename(const std::string& filename);

  void SetNameToSaveConfig();

protected:
  vtkPlusSaveConfigCommand();
  virtual ~vtkPlusSaveConfigCommand();

private:
  std::string Filename;

  vtkPlusSaveConfigCommand(const vtkPlusSaveConfigCommand&);
  void operator=(const vtkPlusSaveConfigCommand&);
};


#endif