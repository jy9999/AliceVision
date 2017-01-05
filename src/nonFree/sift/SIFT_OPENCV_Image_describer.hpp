#ifdef USE_OCVSIFT

#ifndef SIFT_OPENCV_IMAGE_DESCRIBER_HPP
#define	SIFT_OPENCV_IMAGE_DESCRIBER_HPP

/// Feature/Regions & Image describer interfaces
#include "sift.hpp"
#include <openMVG/features/descriptor.hpp>
#include <openMVG/features/image_describer.hpp>
#include <openMVG/features/regions_factory.hpp>

#include <cereal/cereal.hpp>

/// OpenCV Includes
#include <opencv2/opencv.hpp>
#include <opencv2/core/eigen.hpp>
#include <opencv2/xfeatures2d.hpp>

#include <chrono>

namespace openMVG {
namespace features {

#if 0
class SIFT_OPENCV_Params
{
public:
  SIFT_OPENCV_Params() {}
  ~SIFT_OPENCV_Params() {}
  
  bool Set_configuration_preset(EDESCRIBER_PRESET preset)
  {
      switch(preset)
      {
        case LOW_PRESET:
          contrastThreshold = 0.01;
          maxTotalKeypoints = 1000;
          break;
        case MEDIUM_PRESET:
          contrastThreshold = 0.005;
          maxTotalKeypoints = 5000;
          break;
        case NORMAL_PRESET:
          contrastThreshold = 0.005;
          edgeThreshold = 15;
          maxTotalKeypoints = 10000;
          break;
        case HIGH_PRESET:
          contrastThreshold = 0.005;
          edgeThreshold = 20;
          maxTotalKeypoints = 20000;
          break;
        case ULTRA_PRESET:
          contrastThreshold = 0.005;
          edgeThreshold = 20;
          maxTotalKeypoints = 40000;
          break;
      }
      return true;
  }

  template<class Archive>
  void serialize( Archive & ar )
  {
    ar(
      cereal::make_nvp("grid_size", gridSize),
      cereal::make_nvp("max_total_keypoints", maxTotalKeypoints),
      cereal::make_nvp("n_octave_layers", nOctaveLayers),
      cereal::make_nvp("contrast_threshold", contrastThreshold),
      cereal::make_nvp("edge_threshold", edgeThreshold),
      cereal::make_nvp("sigma", sigma));
      // cereal::make_nvp("root_sift", root_sift));
  }

  // Parameters
  std::size_t gridSize = 4;
  std::size_t maxTotalKeypoints = 1000;
  int nOctaveLayers = 3;  // default opencv value is 3
  double contrastThreshold = 0.04;  // default opencv value is 0.04
  double edgeThreshold = 10;
  double sigma = 1.6;
  // bool rootSift = true;
};
#endif

///
//- Create an Image_describer interface that use and OpenCV extraction method
// i.e. with the SIFT detector+descriptor
// Regions is the same as classic SIFT : 128 unsigned char
class SIFT_OPENCV_Image_describer : public Image_describer
{
public:
  SIFT_OPENCV_Image_describer()
    : Image_describer()
    , _verbose( false )
  { }

  ~SIFT_OPENCV_Image_describer() {}

  bool Set_configuration_preset(EDESCRIBER_PRESET preset)
  {
    if( preset != ULTRA_PRESET ) return false;

    return _params.setPreset(preset);
    // return _params.Set_configuration_preset(preset);
  }
  /**
  @brief Detect regions on the image and compute their attributes (description)
  @param image Image.
  @param regions The detected regions and attributes (the caller must delete the allocated data)
  @param mask 8-bit gray image for keypoint filtering (optional).
     Non-zero values depict the region of interest.
  */
  bool Describe(const image::Image<unsigned char>& image,
    std::unique_ptr<Regions> &regions,
    const image::Image<unsigned char> * mask = NULL)
  {
    // Convert for opencv
    cv::Mat img;
    cv::eigen2cv(image.GetMat(), img);

    // Create a SIFT detector
    std::vector< cv::KeyPoint > v_keypoints;
    cv::Mat m_desc;
    // std::size_t maxDetect = 0; // No max value by default
    if(_params._maxTotalKeypoints)
      if(!_params._gridSize)  // If no grid filtering, use opencv to limit the number of features

    if( _verbose ) {
        // std::cout << "maxDetect                 " << maxDetect << std::endl; // to clean
        // std::cout << "_params.nOctaveLayers     " << _params.nOctaveLayers << std::endl;
        // std::cout << "_params.contrastThreshold " << _params.contrastThreshold << std::endl;
        // std::cout << "_params.edgeThreshold     " << _params.edgeThreshold << std::endl;
        std::cout << "_params._maxTotalKeypoints " << _params._maxTotalKeypoints << std::endl; // to clean
        std::cout << "_params._num_octaves       " << _params._num_octaves << std::endl;
        std::cout << "_params._peak_threshold    " << _params._peak_threshold << std::endl;
        std::cout << "_params._edge_threshold    " << _params._edge_threshold << std::endl;
        std::cout << "_params._sigma             " << _params._sigma << std::endl;
    }
    
    // cv::Ptr<cv::Feature2D> siftdetector = cv::xfeatures2d::SIFT::create(maxDetect, _params.nOctaveLayers, _params.contrastThreshold, _params.edgeThreshold, _params.sigma);
    cv::Ptr<cv::Feature2D> siftdetector = cv::xfeatures2d::SIFT::create( _params._maxTotalKeypoints,
                                                                         _params._num_octaves,
                                                                         1.6f * 2.0f * _params._peak_threshold,
                                                                         _params._edge_threshold,
                                                                         _params._sigma );

    // Detect SIFT keypoints
    auto detect_start = std::chrono::steady_clock::now();
    siftdetector->detect(img, v_keypoints);
    auto detect_end = std::chrono::steady_clock::now();
    auto detect_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(detect_end - detect_start);

    if( _verbose ) {
        // std::cout << "SIFT: contrastThreshold: " << _params.contrastThreshold << ", edgeThreshold: " << _params.edgeThreshold << std::endl;
        std::cout << "SIFT: contrastThreshold: " << _params._peak_threshold << ", edgeThreshold: " << _params._edge_threshold << std::endl;
        std::cout << "Detect SIFT: " << detect_elapsed.count() << " milliseconds." << std::endl;
        std::cout << "Image size: " << img.cols << " x " << img.rows << std::endl;
        std::cout << "Grid size: " << _params._gridSize << ", maxTotalKeypoints: " << _params._maxTotalKeypoints << std::endl;
        std::cout << "Number of detected features: " << v_keypoints.size() << std::endl;
    }

    // cv::KeyPoint::response: the response by which the most strong keypoints have been selected.
    // Can be used for the further sorting or subsampling.
    std::sort(v_keypoints.begin(), v_keypoints.end(), [](const cv::KeyPoint& a, const cv::KeyPoint& b) { return a.size > b.size; });

    // Grid filtering of the keypoints to ensure a global repartition
    if(_params._gridSize && _params._maxTotalKeypoints)
    {
      // Only filter features if we have more features than the maxTotalKeypoints
      if(v_keypoints.size() > _params._maxTotalKeypoints)
      {
        std::vector< cv::KeyPoint > filtered_keypoints;
        std::vector< cv::KeyPoint > rejected_keypoints;
        filtered_keypoints.reserve(std::min(v_keypoints.size(), _params._maxTotalKeypoints));
        rejected_keypoints.reserve(v_keypoints.size());

        cv::Mat countFeatPerCell(_params._gridSize, _params._gridSize, cv::DataType<std::size_t>::type, cv::Scalar(0));
        const std::size_t keypointsPerCell = _params._maxTotalKeypoints / countFeatPerCell.total();
        const double regionWidth = image.Width() / double(countFeatPerCell.cols);
        const double regionHeight = image.Height() / double(countFeatPerCell.rows);

        if( _verbose ) {
            std::cout << "Grid filtering -- keypointsPerCell: " << keypointsPerCell
                      << ", regionWidth: " << regionWidth
                      << ", regionHeight: " << regionHeight << std::endl;
        }

        for(const cv::KeyPoint& keypoint: v_keypoints)
        {
          const std::size_t cellX = std::min(std::size_t(keypoint.pt.x / regionWidth), _params._gridSize);
          const std::size_t cellY = std::min(std::size_t(keypoint.pt.y / regionHeight), _params._gridSize);
          // std::cout << "- keypoint.pt.x: " << keypoint.pt.x << ", keypoint.pt.y: " << keypoint.pt.y << std::endl;
          // std::cout << "- cellX: " << cellX << ", cellY: " << cellY << std::endl;
          // std::cout << "- countFeatPerCell: " << countFeatPerCell << std::endl;
          // std::cout << "- gridSize: " << _params._gridSize << std::endl;

          const std::size_t count = countFeatPerCell.at<std::size_t>(cellX, cellY);
          countFeatPerCell.at<std::size_t>(cellX, cellY) = count + 1;
          if(count < keypointsPerCell)
            filtered_keypoints.push_back(keypoint);
          else
            rejected_keypoints.push_back(keypoint);
        }
        // If we don't have enough features (less than maxTotalKeypoints) after the grid filtering (empty regions in the grid for example).
        // We add the best other ones, without repartition constraint.
        if( filtered_keypoints.size() < _params._maxTotalKeypoints )
        {
          const std::size_t remainingElements = std::min(rejected_keypoints.size(), _params._maxTotalKeypoints - filtered_keypoints.size());
          if( _verbose ) {
            std::cout << "Grid filtering -- Copy remaining points: " << remainingElements << std::endl;
          }
          filtered_keypoints.insert(filtered_keypoints.end(), rejected_keypoints.begin(), rejected_keypoints.begin() + remainingElements);
        }

        v_keypoints.swap(filtered_keypoints);
      }
    }
    if( _verbose ) {
        std::cout << "Number of features: " << v_keypoints.size() << std::endl;
    }

    // Compute SIFT descriptors
    auto desc_start = std::chrono::steady_clock::now();
    siftdetector->compute(img, v_keypoints, m_desc);
    auto desc_end = std::chrono::steady_clock::now();
    auto desc_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(desc_end - desc_start);
    if( _verbose ) {
        std::cout << "Compute descriptors: " << desc_elapsed.count() << " milliseconds." << std::endl;
    }

    Allocate(regions);

    // Build alias to cached data
    SIFT_Regions * regionsCasted = dynamic_cast<SIFT_Regions*>(regions.get());
    // reserve some memory for faster keypoint saving
    regionsCasted->Features().reserve(v_keypoints.size());
    regionsCasted->Descriptors().reserve(v_keypoints.size());

    // Prepare a column vector with the sum of each descriptor
    cv::Mat m_siftsum;
    cv::reduce(m_desc, m_siftsum, 1, cv::REDUCE_SUM);

    // Copy keypoints and descriptors in the regions
    int cpt = 0;
    for(std::vector< cv::KeyPoint >::const_iterator i_kp = v_keypoints.begin();
        i_kp != v_keypoints.end();
        ++i_kp, ++cpt)
    {
      SIOPointFeature feat((*i_kp).pt.x, (*i_kp).pt.y, (*i_kp).size, (*i_kp).angle);
      regionsCasted->Features().push_back(feat);

      Descriptor<unsigned char, 128> desc;
      for(int j = 0; j < 128; j++)
      {
        desc[j] = static_cast<unsigned char>(512.0*sqrt(m_desc.at<float>(cpt, j)/m_siftsum.at<float>(cpt, 0)));
      }
      regionsCasted->Descriptors().push_back(desc);
    }

    return true;
  };

  /// Allocate Regions type depending of the Image_describer
  void Allocate(std::unique_ptr<Regions> &regions) const
  {
    regions.reset( new SIFT_Regions );
  }

  template<class Archive>
  void serialize( Archive & ar )
  {
    ar(cereal::make_nvp("params", _params));
  }

private:
#if 0
  SIFT_OPENCV_Params _params;
#else
  SiftParams _params;
#endif
  bool _verbose;
};

} // namespace features
} // namespace openMVG

CEREAL_REGISTER_TYPE_WITH_NAME(openMVG::features::SIFT_OPENCV_Image_describer, "SIFT_OPENCV_Image_describer");

#endif	/* SIFT_OPENCV_IMAGE_DESCRIBER_HPP */

#endif //USE_OCVSIFT

