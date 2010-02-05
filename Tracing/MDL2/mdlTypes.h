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
#ifndef __mdlTypes_h
#define __mdlTypes_h

#include "itkImage.h"

namespace mdl
{
	typedef struct {int x; int y; int z;} Point3D;
	typedef struct {float x; float y; float z;} fPoint3D;

	typedef unsigned char PixelType;
    static const unsigned int Dimension = 3;
    typedef itk::Image< PixelType, Dimension > ImageType;

}  // end namespace mdl

#endif
