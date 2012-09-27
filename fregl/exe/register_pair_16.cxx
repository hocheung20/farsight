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

//: Executable program to register a pair of 3D images 
// 
//  The outcome is an xml file of a registration record. The images
//  can be gray, rgb color or rgba color. The input images are
//  assumed TIF images. The useage is
//
//   register_pair from_image_name to_image_name
//
//
// where 
//  from_image_name      name of the from_image
//  to_image_name        name of the to_image
// 
// optional arguments:
//  -channel        The channel to be extracted.
//  -background     Intensities below -background are ignored.
//  -smooth         Gaussian smoothing is required
//  -prior          xml file containing the prior transformation for initialization 

#include <iostream>
using std::cerr;
using std::endl;

#include "itkImageFileReader.h"
#include "itkCastImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkDiscreteGaussianImageFilter.h"
#include "itkRGBAPixel.h"
#include "itkRGBPixel.h"
#include "itkImage.h"
#include "itkTIFFImageIO.h"
#include "itkImageFileWriter.h"

#include <vul/vul_file.h>
#include <vul/vul_arg.h>
#include <vul/vul_timer.h>

#include <fregl/fregl_pairwise_register.h>
#include <fregl/fregl_joint_register.h>
#include <fregl/fregl_reg_record.h>
#include <fregl/fregl_util.h>

typedef unsigned short                   InputPixelType;
typedef itk::Image< InputPixelType, 3 > ImageType3D;
typedef float                           InternalPixelType;
typedef itk::Image<InternalPixelType, 3 > InternalImageType;

ImageType3D::Pointer  
smooth_image(ImageType3D::Pointer image, int num_sub_images )
{

	typedef itk::DiscreteGaussianImageFilter< ImageType3D,InternalImageType > SmoothingFilterType;

	typedef itk::CastImageFilter< InternalImageType,ImageType3D > CastFilterType;
	typedef itk::RescaleIntensityImageFilter< ImageType3D, ImageType3D> RescalerType;
	typedef itk::StreamingImageFilter<ImageType3D, ImageType3D> StreamingFilterType;

	SmoothingFilterType::Pointer smoother = SmoothingFilterType::New();
	RescalerType::Pointer rescaler = RescalerType::New();
	CastFilterType::Pointer caster = CastFilterType::New();
	StreamingFilterType::Pointer streamer = StreamingFilterType::New();

	smoother->SetInput( image );
	smoother->SetVariance(1.0);
	smoother->SetMaximumKernelWidth(15);

	caster->SetInput(smoother->GetOutput());

	rescaler->SetInput( caster->GetOutput() );
	rescaler->SetOutputMinimum( 0 );
	rescaler->SetOutputMaximum( 250 );

	streamer->SetInput( rescaler->GetOutput() );
	streamer->SetNumberOfStreamDivisions(num_sub_images);
	try {
		streamer->Update();
	}
	catch(itk::ExceptionObject& e) {
		vcl_cout << e << vcl_endl;
	}

	return streamer->GetOutput();
}

int
main(  int argc, char* argv[] )
{
	vul_arg< vcl_string > arg_file_from  ( 0, "From image file" );
	vul_arg< vcl_string > arg_file_to    ( 0, "To image file" );
	vul_arg< int >       channel         ("-channel", "The color channel (0-red, 1-green, 2-blue), or the image channel if the original image is a lsm image.",0);
	vul_arg< float >     background      ("-bg", "threshold value for the background", 30);
	vul_arg< double >      smooth        ("-smooth", "If smoothing is performed", 0.5);
	//vul_arg< int >       streaming       ("-streaming","Apply streaming with the number of sub-images when smooth is set", 1);
	//vul_arg< bool >     exhaustive      ("-exhaustive", "Running exhaustive search", false);
	vul_arg< vcl_string > gdbicp ("-gdbicp","The path where the gdbicp exe can be found. The exe of gdbicp can be found at http://www.vision.cs.rpi.edu/gdbicp","");
	vul_arg< int >         slices         ("-slices","Number of slices to register. Needed when registering large image stacks on PC.",100);
	vul_arg<bool>          remove_2d      ("-remove_2d", "Remove the intermediate 2d stuff", false);
	vul_arg<bool>          scaling_arg    ("-scaling","Substantial scaling is expected.", false);
	vul_arg< vcl_string >  prior_arg    ("-prior","xml file containing the initial transformation");

	vul_arg_parse( argc, argv );

	ImageType3D::Pointer from_image, to_image;

	// Read the image
	//
	//
	std::cout<<"Image pair: "<<arg_file_from()<<" to "<<arg_file_to()<<std::endl;
	from_image = fregl_util< InputPixelType >::fregl_util_read_image( arg_file_from(), channel.set(), channel() );
	to_image = fregl_util< InputPixelType >::fregl_util_read_image( arg_file_to(), channel.set(), channel() );
	if (!from_image || !to_image) {
		vcl_cerr <<"Failed to read image(s)"<< vcl_endl;
	}
	/*
	typedef itk::ImageFileWriter< ImageType >  WriterType3D;
	WriterType3D::Pointer writer3D = WriterType3D::New();
	writer3D->SetFileName( "from.tiff" );
	writer3D->SetInput( from_image );
	writer3D->Update();
	writer3D->SetFileName( "to.tiff" );
	writer3D->SetInput( to_image );
	writer3D->Update();
	*/

	std::string from_image_id = vul_file::strip_directory( arg_file_from() );
	std::string to_image_id = vul_file::strip_directory( arg_file_to() );
	std::string from_image_id_wo_ext = vul_file::strip_extension( from_image_id );
	std::string to_image_id_wo_ext = vul_file::strip_extension( to_image_id );

	// Perform registration
	//
	fregl_pairwise_register< InputPixelType > registor(from_image, to_image, from_image_id_wo_ext, to_image_id_wo_ext, background());
	if (slices.set()) registor.set_stack_size( slices() );
	registor.set_smoothing( smooth() );

	// If initial transformation is required, get the record from the xml file
	fregl_util< InputPixelType >::TransformType::Pointer prior_xform;
	if (prior_arg.set()) {
		fregl_joint_register< InputPixelType >::Pointer joint_register = new fregl_joint_register< InputPixelType >(prior_arg());
		prior_xform = joint_register->get_transform(from_image_id, to_image_id);
	}

	double obj_value;
	bool succeeded = false;
	vul_timer timer;
	timer.mark();
	if (prior_arg.set()) {
		if (registor.run(prior_xform, obj_value))
			succeeded = true;
	}
	else if (registor.run(obj_value, gdbicp(), scaling_arg()))
		succeeded = true;

	if (succeeded) {
		std::cout << "Timing: Successful registration in  ";
		timer.print( std::cout );
		std::cout<<std::endl;

		// Set the registration result
		ImageType3D::SizeType from_image_size = from_image->GetLargestPossibleRegion().GetSize();
		ImageType3D::SizeType to_image_size = to_image->GetLargestPossibleRegion().GetSize();


		// Invert the transforn, since itk produces xform going from
		// to->from image
		typedef fregl_pairwise_register< InputPixelType >::TransformType TransformType;
		TransformType::Pointer xform = registor.transform();
		TransformType::Pointer inv_xform = TransformType::New();
		xform->GetInverse(inv_xform);

		//Create the reg_record and write to xml output
		fregl_reg_record::Pointer record = new fregl_reg_record();
		record->set_from_image_info(from_image_id, from_image_size);
		record->set_to_image_info(to_image_id, to_image_size);
		record->set_transform( inv_xform );
		record->set_obj_value( obj_value );
		double vol_overlap = fregl_util< InputPixelType >::fregl_util_overlap(inv_xform, from_image_size,
			to_image_size);
		record->set_overlap(vol_overlap);
		std::string xml_file = from_image_id_wo_ext+std::string("_to_")+
			to_image_id_wo_ext+std::string("_transform.xml");
		record->write_xml( xml_file );
	}
	else {
		std::cout<<"Timing: Failed registration in ";
		timer.print( std::cout );
		std::cout<<std::endl;
		return 1;
	}

	if (!prior_arg.set() && remove_2d()) {
#if defined(VCL_WIN32) && !defined(__CYGWIN__)
		//Delete temporary files, no need to delete now since they are unique
		vcl_system(("del " + from_image_id_wo_ext + "_to_" + to_image_id_wo_ext + "_xxx_" + from_image_id_wo_ext + "_proj.tif").c_str());
		vcl_system(("del " + from_image_id_wo_ext + "_to_" + to_image_id_wo_ext + "_xxx_" + to_image_id_wo_ext + "_proj.tif").c_str());
		vcl_system(("del mosaic_" + from_image_id_wo_ext + "_to_" + to_image_id_wo_ext + "_xxx_" + from_image_id_wo_ext + "_proj_to_" + from_image_id_wo_ext + "_to_" + to_image_id_wo_ext + "_xxx_" + to_image_id_wo_ext + "_proj.xform").c_str());
#else
		if( vcl_system("rm xxx_from_image_proj.tif") != 0 )
		{
			cerr << "The command 'rm xxx_from_image_proj.tif' returned nonzero" << endl;
		}
		if( vcl_system("rm xxx_to_image_proj.tif") != 0 )
		{
			cerr << "The command 'rm xxx_to_image_proj.tif' returned nonzero" << endl;
		}
		if( vcl_system("rm mosaic_xxx_from_image_proj_to_xxx_to_image_proj.xform") != 0 )
		{
			cerr << "The command 'rm mosaic_xxx_from_image_proj_to_xxx_to_image_proj.xform' returned nonzero" << endl;
		}
#endif
	}

	if (!succeeded) return 1;

	return 0;
}