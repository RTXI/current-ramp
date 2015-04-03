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

#include <default_gui_model.h>
#include <DataLogger.cpp>
#include <string>

class Iramp : public DefaultGUIModel {

	public:
		Iramp(void);
		virtual ~Iramp(void);
		virtual void execute(void);

	protected:
		virtual void update(DefaultGUIModel::update_flags_t);

	private:
#define EPS 1e-9
		double V, Iout;

		double dt;
		double rate;
		double maxt;
		double Istart, Iend;

		int active;
		int peaked;

		// DataLogger
		DataLogger data;
		int acquire;
		double tcnt;
		int cellnum;
		string prefix, info;
		vector<double> newdata;
};
