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

//: 
// \file
// \brief The class manages the global image in a manner similar to
//  an itk image reader.
// \author Jay Koven
// \date 11/11/2011

#ifndef _fregl_image_manager_h_
#define _fregl_image_manager_h_

#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>
#include <vnl/vnl_vector_fixed.h>
#include <vector>
#include <string>
#include "pthread.h"

#include "itkIndex.h"
#include "itkSize.h"
#include "itkPoint.h"
#include "itkImage.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include <vul/vul_file.h>

#include <fregl/fregl_joint_register.h>
#include <fregl/fregl_space_transformer.h>

class fregl_image_manager : public vbl_ref_count {
public:
    typedef vbl_smart_ptr< fregl_image_manager > Pointer;
    typedef itk::Image<unsigned int, 3 > ImageType;
    typedef ImageType::PointType PointType; //physical space
    typedef ImageType::SizeType SizeType;
    typedef ImageType::SpacingType SpacingType;
    typedef ImageType::IndexType IndexType;
    typedef itk::Image< float, 2 > FloatImageType2D;

    //: Constructor
    // xml_filename - joint register xml file
    // image_path - path to image files
    // anchor_image - image to use to define the global space
    // use_NN - Use nearest neighbor for image transformations.
    fregl_image_manager(std::string const & xml_filename, std::string const & image_path, std::string const & anchor_image,
            bool use_NN = false);

    ~fregl_image_manager() {
    }

    //: Set the Region of Interest
    //
    // Set the region of interest in the global (anchor) space
    //
    //  Origin  and size are updated accordingly.
    void set_regionofinterest(PointType origin, SizeType size);

    //: Set the Region of Interest
    //
    // Set region of interest in the global (anchor) space
    //
    //  Index and size are updated accordingly.
    void set_regionofinterest(IndexType origin, SizeType size);

    //: Set the image channel to montage
    //
    //  If not set channels are fused!!
    void set_channel(int channel);

    //:  Build the Montage based on the current Region
    //
    //   Builds region of interest image in the global space using
    //   "s_origin" is the new roi origin point
    //   "s_size" is the new roi size
    void Update();

    //: Return an ITK image pointer to the current Montage of
    //  region of interest.  Pointer points to clone of image
    //  so image can be modified with out affect.
    //
    // Montage coordinates are in Normalized space (0,0) space.
    //
    ImageType::Pointer GetOutput();
    
    //: Return an ITK image pointer the the region of interest passed an argument
    // This entry is mutex protected for multi threading operations.  It combines
    // the set region, update and get output methods.
    ImageType::Pointer MutexGetRegionOfInterest(PointType origin, SizeType size);

    //: Return an ITK image pointer to the Region of Interest in the passed
    //  file name.  Return from cache if file is cached otherwise read the file
    //  into cache then get the region.
    void ReadFileRegion(std::string file_name, int image_index, ImageType::Pointer montage_image);

    //: Return the global (anchor) space origin
    //
    PointType get_global_origin();

    //: Return return the global (anchor) space size
    //
    SizeType get_global_size();

    //: Return the current Region origin in global space
    //
    PointType get_region_origin();

    //: Return the current Region Size
    //
    SizeType get_region_size();

    //: Return the anchor image name
    // Not sure why this is here
    std::string const get_anchor_name();

    //: Return a pointer the space transformer
    fregl_space_transformer::Pointer get_space_transformer();
    
    //: Set up the cache buffer count.  If 0 Turn off caching
    // Will reset all cache buffers when called.
    void set_cache_buffer_count(int count);

    //: Set disk caching on or off.  
    void set_file_caching(bool use);
    
    //: Set the directory for the disk caching.  The default is current
    // directory "."
    void set_file_cache_dir(std::string use_dir);



private:
    int get_next_slot();
    bool cache_write_image(int image_index, ImageType::Pointer t_image);
    ImageType::Pointer cache_read_image(int image_index);
    
    fregl_joint_register::Pointer global_joint_register;
    fregl_space_transformer::Pointer global_space_transformer;

    // The physical space is defined by the reference image
    PointType global_origin;
    SizeType global_size;
    IndexType roi_index;
    PointType roi_origin;
    SizeType roi_size;
    SpacingType global_spacing_;
    std::vector<std::string> image_names;
    std::string global_anchor;
    std::string global_image_path;
    bool use_NN_interpolator;
    int global_channel;
    bool global_use_channel;
    ImageType::Pointer montage_image;
    std::vector<bool> is_cached;
    std::vector<bool> is_cached_on_disk;
    std::vector<ImageType::Pointer> cached_images;
    std::vector<int> cache_slot;
    std::vector<int> cache_last_used;
    int cache_time;
    int cache_size;
    bool use_caching;
    bool use_file_caching;
    std::string cache_dir;
    pthread_mutex_t region_mutex;


};
#endif
