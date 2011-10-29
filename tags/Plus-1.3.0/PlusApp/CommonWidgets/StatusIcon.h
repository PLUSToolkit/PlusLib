/*=Plus=header=begin======================================================
  Program: Plus
  Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
  See License.txt for details.
=========================================================Plus=header=end*/ 

#ifndef STATUSICON_H
#define STATUSICON_H

#include <QWidget>

#include "PlusConfigure.h"

#include <QLabel>
#include <QTextEdit>

#include "vtkCallbackCommand.h"

//-----------------------------------------------------------------------------

/*! \class vtkDisplayMessageCallback 
 *
 * \brief Callback command class that catches the log message event and adds it to the text field of the status icon
 *
 * \ingroup PlusAppCommonWidgets
 *
 */
class vtkDisplayMessageCallback : public QObject, public vtkCallbackCommand
{
  Q_OBJECT

public:
	static vtkDisplayMessageCallback *New()
	{
		vtkDisplayMessageCallback *cb = new vtkDisplayMessageCallback();
		return cb;
	}

  vtkDisplayMessageCallback::vtkDisplayMessageCallback()
    : QObject()
  { }

	virtual void Execute(vtkObject *caller, unsigned long eventId, void *callData)
	{
    if (vtkCommand::UserEvent == eventId) {
      char* callDataChars = reinterpret_cast<char*>(callData);

      emit AddMessage(QString::fromAscii(callDataChars));
    }
	}

signals:
  void AddMessage(QString);
};
 
//-----------------------------------------------------------------------------

/*! \class StatusIcon 
 *
 * \brief Widget that shows the current status of the application and displays all log messages occurred
 *
 * \ingroup PlusAppCommonWidgets
 *
 */
class StatusIcon : public QWidget
{
  Q_OBJECT

public:
  /*!
  * \brief Constructor
  * \param aParent parent
  * \param aFlags widget flag
  */
  StatusIcon(QWidget* aParent = 0, Qt::WFlags aFlags = 0);

  /*!
  * \brief Destructor
  */
  ~StatusIcon();

protected:
  /*!
  * \brief Filters events if this object has been installed as an event filter for the watched object
  * \param obj object
  * \param ev event
  * \return if you want to filter the event out, i.e. stop it being handled further, return true; otherwise return false
  */
  bool eventFilter(QObject *obj, QEvent *ev);

  /*!
  * \brief Creates message frame and fills up message field with the massages
  * \return Success flag
  */
  PlusStatus ConstructMessageListWidget();

public slots:
  /*!
  * \brief Callback function for logger to display messages in popup window
  * \return aInputString Log level and message in a string
  */
  void AddMessage(QString aInputString);

protected:
  //! State level of the widget ( no errors (>2): green , warning (2): orange , error (1): red )
  int							            m_Level;

  //! Label representing the colored dot for of this widget
  QLabel*					          	m_DotLabel;

  //! Frame containing the field of messages
  QFrame*						          m_MessageListWidget;

  //! Field containing the messages
  QTextEdit*					        m_MessageTextEdit;

  //! Tag number of the display message callback
  unsigned long               m_DisplayMessageCallbackTag;
};

#endif
