/*=========================================================================
Copyright 2009 Rensselaer Polytechnic Institute
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. 
=========================================================================*/

#ifndef ALPHA_EXPANSION_H
#define ALPHA_EXPANSION_H

#include <iostream>
#include "GraphCut.h"
#include "GCoptimization.h"
#include "Multi_Color_Graph_Learning_2D.h"

void start_alpha_expansion(float* im, unsigned short* seg_im, float* Dterms, int R, int C, int Z, int K);
void alpha_expansion_2d( float *im, float *sublogImg, unsigned short *subclustImg, int R, int C);

#endif

