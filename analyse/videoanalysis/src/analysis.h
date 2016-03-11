/* alysis.h
 *
 * Copyright (C) 2016 freyr <sky_rider_93@mail.ru> 
 *
 * This file is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 3 of the 
 * License, or (at your option) any later version. 
 *
 * This file is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
*/

#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "videodata.h"
#include <stdlib.h>

inline void
analyse_buffer(guint8* data,
	       guint8* data_prev,
	       guint stride,
	       guint width,
	       guint height,
	       guint black_bnd,
	       guint freez_bnd,
	       VideoParams *rval)
{
  rval->avg_bright = .0;
  rval->avg_diff = .0;
  rval->blocks = .0;
  rval->black_pix = .0;
  rval->frozen_pix = .0;

  long brightness = 0;
  long difference = 0;
  guint black = 0;
  guint frozen = 0;
  double Shblock = 0;
  double Shnonblock = 0;
  
  for (guint j = 0; j < height; j++) {
    for (guint i = 0; i < width; i++) {
      int ind = i + j*stride;
      guint8 current = data[ind];
      guint8 diff = 0;
      if (i && !(i % 4)) {
	double sum;
	guint8 sub, subNext, subPrev;
	int ind = i + j*stride;
	sub = abs(data[ind - 1] - current);
	subNext = abs(current - data[ind + 1]); 
	subPrev = abs(data[ind - 2] - data[ind - 1]);
	sum = (double)subNext + (double)subPrev;
	if (sum == .0) sum = 1.0;
	else sum = sum / 2.0; 
	if (!(i % 8)){
	  Shnonblock += (double)sub/sum;
	}
	else {
	  Shblock += (double)sub/sum;
	}
      }
      brightness += current;
      black += (current <= black_bnd) ? 1 : 0;
      if (data_prev != NULL){
	guint8 current_prev = data_prev[ind];
	diff = abs(current - current_prev);
	difference += diff;
	frozen += (diff <= freez_bnd) ? 1 : 0;
	data_prev[ind] = current;
      }
    }
  }
  if (!Shnonblock) Shnonblock = 4;
  
  rval->blocks = Shnonblock/Shblock;
  rval->avg_bright = (float)brightness / (height*width);
  rval->black_pix = ((float)black/((float)height*(float)width))*100.0;
  rval->avg_diff = (float)difference / (height*width);
  rval->frozen_pix = (frozen/(height*width))*100.0;
}

#endif /* ANALYSIS_H */
