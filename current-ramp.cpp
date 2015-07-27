/*
 * Copyright (C) 2004 Boston University
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "current-ramp.h"

Iramp::ToggleRampEvent::ToggleRampEvent(Iramp *pt, bool rp, bool rd) {
	parent = pt;
	ramping = rp;
	recording = rd;
}

Iramp::ToggleRampEvent::~ToggleRampEvent(void) {}

int Iramp::ToggleRampEvent::callback(void) {
	if (ramping) {
		if (recording) {
			::Event::Object event(::Event::START_RECORDING_EVENT);
			::Event::Manager::getInstance()->postEventRT(&event);
			parent->acquire = 1;
		}
		parent->active = 1;
		parent->peaked = 0;
		parent->done = false;
	} else {
//		parent->acquire = 1;
		parent->active = 0;
		parent->done = true;
	}
	return 0;
}

extern "C" Plugin::Object *createRTXIPlugin(void) {
	return new Iramp();
}

static DefaultGUIModel::variable_t vars[] = {
	{
		"Vin",
		"",
		DefaultGUIModel::INPUT,
	},
	{
		"Iout",
		"",
		DefaultGUIModel::OUTPUT,
	},
	{
		"Start Amp (pA)",
		"",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
	},
	{
		"End Amp (pA)",
		"",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
	},
	{
		"Time (s)",
		"",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
	},
	{
		"Cell (#)",
		"",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::UINTEGER,
	},
	{
		"tcnt",
		"",
		DefaultGUIModel::STATE,
	},
	{
		"V",
		"",
		DefaultGUIModel::STATE,
	},
	{
		"Iout (pA)",
		"",
		DefaultGUIModel::STATE,
	},
};

static size_t num_vars = sizeof(vars)/sizeof(DefaultGUIModel::variable_t);

Iramp::Iramp(void) : DefaultGUIModel("Current Ramp",::vars,::num_vars), 
                     dt(RT::System::getInstance()->getPeriod()*1e-6), 
                     maxt(30.0), Istart(0.0), Iend(100.0), active(0), 
                     acquire(0), cellnum(1), peaked(0), done(true) {

	DefaultGUIModel::createGUI(vars, num_vars);
	update(INIT);
	customizeGUI();
	refresh();
	QTimer::singleShot(0, this, SLOT(resizeMe()));
}

Iramp::~Iramp(void) {}

void Iramp::execute(void) {
	V = input(0);

	if (active) {
		if (!peaked) {
			if (Iout<Iend) {
				Iout+=rate*dt/1000;
			} else {
				Iout-=rate*dt/1000;
				peaked = 1;
			}
		} else {
			if (Iout >= Istart-EPS) { //EPS is a macro defined in header
				Iout-=rate*dt/1000;
			} else {
				Iout = 0;
				active = 0;
				peaked = 0;
			}
		}
	}

	//Do data logging and data writing
	if (acquire && active) {
		tcnt+=dt/1000;
	}
	else if (acquire && !active) {
		tcnt = 0;
		acquire = 0;
		::Event::Object event(::Event::STOP_RECORDING_EVENT);
		::Event::Manager::getInstance()->postEventRT(&event);
	}

	output(0) = Iout*1e-12;
}

void Iramp::update(DefaultGUIModel::update_flags_t flag) {

	switch(flag) {
	case INIT:
		setParameter("Time (s)", maxt);
		setParameter("Start Amp (pA)", Istart);
		setParameter("End Amp (pA)", Iend);
		setParameter("Cell (#)", cellnum);

		setState("tcnt", tcnt);
		setState("V", V);
		setState("Iout", Iout);
		break;

	case MODIFY:
		maxt   = getParameter("Time (s)").toDouble();
		Istart = getParameter("Start Amp (pA)").toDouble();
		Iend   = getParameter("End Amp (pA)").toDouble();
		cellnum = getParameter("Cell (#)").toInt();

		//Reset ramp
		Iout = Istart*active;

		peaked = 0;
		rate = (Iend-Istart)/maxt/2; //In (pA/sec)
		tcnt = 0;
		
		acquire = recordBox->isChecked();
		active = rampButton->isChecked();
		break;

	case PERIOD:
		dt = RT::System::getInstance()->getPeriod()*1e-6;
		break;

	case PAUSE:
		output(0) = 0.0;
		rampButton->setEnabled(false);
		break;

	case UNPAUSE:
		rampButton->setEnabled(true);
		break;

	default:
		break;
	}
}

void Iramp::customizeGUI(void) {
	QGridLayout * customLayout = DefaultGUIModel::getLayout();

	QGroupBox *acquireBox = new QGroupBox;
	QHBoxLayout *acquireBoxLayout = new QHBoxLayout;
	acquireBox->setLayout(acquireBoxLayout);

	rampButton = new QPushButton("RAMP!!");
	rampButton->setStyleSheet("font-weight:bold;font-style:italic;");
	rampButton->setCheckable(true);
	acquireBoxLayout->addWidget(rampButton);

//	acquireBoxLayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding, QSizePolicy::Minimum));

	recordBox = new QCheckBox("Record Data");
	acquireBoxLayout->addWidget(recordBox);

	customLayout->addWidget(acquireBox, 0, 0);
	setLayout(customLayout);

	QObject::connect(recordBox, SIGNAL(clicked(void)), this, SLOT(modify(void)));
	QObject::connect(rampButton, SIGNAL(clicked(void)), this, SLOT(toggleRamp(void)));

	rampCheckTimer = new QTimer(this);
	QTimer::connect(rampCheckTimer, SIGNAL(timerout(void)), this, SLOT(rampTimerFunction(void)));
	rampCheckTimer->start(1000);
}

void Iramp::toggleRamp(void) {
	ToggleRampEvent event(this, rampButton->isChecked(), recordBox->isChecked());
	RT::System::getInstance()->postEvent( &event );
}

void Iramp::rampTimerFunction(void) {
	if ( done && rampButton->isChecked() ) {
		rampButton->disconnect();
		rampButton->setChecked(false);
		QObject::connect(rampButton, SIGNAL(clicked(void)), this, SLOT(toggleRamp(void)));
	}
}

void Iramp::receiveEvent( const Event::Object *event ) {
   if( event->getName() == Event::START_RECORDING_EVENT ) acquire = true;
   if( event->getName() == Event::STOP_RECORDING_EVENT ) acquire = false;
}

void Iramp::receiveEventRT( const Event::Object *event ) {
   if( event->getName() == Event::START_RECORDING_EVENT ) acquire = true;
   if( event->getName() == Event::STOP_RECORDING_EVENT ) acquire = false;
}
