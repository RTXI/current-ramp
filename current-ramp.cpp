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

#include <current-ramp.h>

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
		"Time (sec)",
		"",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
	},
	{
		"Active?",
		"",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::UINTEGER,
	},
	{
		"Acquire?",
		"",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::UINTEGER,
	},
	{
		"Cell (#)",
		"",
		DefaultGUIModel::PARAMETER | DefaultGUIModel::UINTEGER,
	},
	{
		"File Prefix",
		"",
		DefaultGUIModel::COMMENT //| DefaultGUIModel::DOUBLE,
	},
	{
		"File Info",
		"",
		DefaultGUIModel::COMMENT //| DefaultGUIModel::DOUBLE,
	},
};

static size_t num_vars = sizeof(vars)/sizeof(DefaultGUIModel::variable_t);

Iramp::Iramp(void) : DefaultGUIModel("Current Ramp",::vars,::num_vars), dt(RT::System::getInstance()->getPeriod()*1e-6), maxt(30.0), Istart(0.0), Iend(100.0), active(0), acquire(0), cellnum(1), prefix("Iramp"), info("n/a") {

	newdata.push_back(0);
	newdata.push_back(0);
	newdata.push_back(0);

	DefaultGUIModel::createGUI(vars, num_vars);
	update(INIT);
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
			if (Iout >= Istart-EPS) {
				Iout-=rate*dt/1000;
			} else {
				Iout = 0;
				active = 0;
				peaked = 0;
				setParameter("Active?", active);
				refresh();
			}
		}
	}

	//Do data logging and data writing
	if (acquire && active) {
		newdata[0] = tcnt;
		newdata[1] = V;
		newdata[2] = Iout*1e-12;
		data.insertdata(newdata);
		tcnt+=dt/1000;
	}
	else if (acquire && !active) {
		data.writebuffer(prefix, info);
		data.resetbuffer();

		tcnt = 0;
		acquire = 0;
		setParameter("Acquire?", acquire);
		refresh();
	}

	output(0) = Iout*1e-12;
}

void Iramp::update(DefaultGUIModel::update_flags_t flag) {

	switch(flag) {
	case INIT:
		setParameter("Time (sec)",maxt);
		setParameter("Start Amp (pA)",Istart);
		setParameter("End Amp (pA)",Iend);
		setParameter("Active?",active);

		setParameter("Acquire?",acquire);
		setParameter("Cell (#)",cellnum);
		setComment("File Prefix", QString::fromStdString(prefix));
		setComment("File Info", QString::fromStdString(info));
		break;

	case MODIFY:
		maxt   = getParameter("Time (sec)").toDouble();
		Istart = getParameter("Start Amp (pA)").toDouble();
		Iend   = getParameter("End Amp (pA)").toDouble();
		active = getParameter("Active?").toInt();

		acquire = getParameter("Acquire?").toInt();
		cellnum = getParameter("Cell (#)").toInt();
		prefix = getComment("File Prefix").toStdString();
		info = getComment("File Info").toStdString();

		//Reset ramp
		Iout = Istart*active;

		peaked = 0;
		rate = (Iend-Istart)/maxt/2; //In (pA/sec)
		tcnt = 0;
		
		//Reset data saving stuff
		data.newcell(cellnum);
		data.resetbuffer();
		break;

	case PERIOD:
		dt = RT::System::getInstance()->getPeriod()*1e-6;

	case PAUSE:
		output(0) = 0.0;

	default:
		break;
	}
}
