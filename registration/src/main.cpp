#include <opencv2/opencv.hpp>
#include "opencv2/features2d.hpp"

void stack(cv::Mat src, cv::Mat lyr, cv::Mat &out);
void makeTransparent(cv::Mat src, cv::Mat &srcTransparent);
void circleDetectCrop(cv::Mat src, cv::Mat &srcCropped, std::string path);
void writeDetectedCircles(
    cv::Mat src,
    std::vector<cv::Vec3f> circles,
    std::string path
);
void getHomography(cv::Mat src, cv::Mat trg, cv::Mat &homography, std::string path);
void writeMatchedPoints(
    std::vector<uchar> inliers_mask,
    std::vector<cv::DMatch> good_matches,
    cv::Mat src, std::vector<cv::KeyPoint> srcKeypoints,
    cv::Mat trg, std::vector<cv::KeyPoint> trgKeypoints,
    std::string path
);

int main(int argc, char* argv[]) {
    std::string timestamp = argv[1];
    std::string path = "../Images/" + timestamp;
    cv::Mat src, trg, lyr;

    src = cv::imread(
        cv::samples::findFile(path + "/user.png"),
        cv::IMREAD_COLOR
    );
    trg = cv::imread(
        cv::samples::findFile(path + "/real.png"),
        cv::IMREAD_COLOR
    );
    lyr = cv::imread(
        cv::samples::findFile(path + "/layer.png"),
        cv::IMREAD_COLOR
    );

    cv::Mat srcCropped, srcWarped, trgSized, homography,
        lyrSized, lyrTransparent, lyrWarped, stacked;

    circleDetectCrop(src, srcCropped, path);

    resize(
        trg,
        trgSized,
        cv::Size(srcCropped.cols, srcCropped.rows),
        cv::INTER_LINEAR
    );
    resize(
        lyr,
        lyrSized,
        cv::Size(srcCropped.cols, srcCropped.rows),
        cv::INTER_LINEAR
    );

    getHomography(srcCropped, trgSized, homography, path);

    cv::warpPerspective(
        srcCropped,
        srcWarped,
        homography,
        srcCropped.size()
    );
    cv::warpPerspective(
        lyrSized,
        lyrWarped,
        homography.inv(),
        lyrSized.size()
    );

    makeTransparent(lyrWarped, lyrTransparent);
    stack(srcCropped, lyrTransparent, stacked);

    // cv::imwrite(path + "/warped-user.png", srcWarped);
    // cv::imwrite(path + "/warped-layer.png", lyrTransparent);
    cv::imwrite(path + "/cropped.png", srcCropped);
    cv::imwrite(path + "/stacked.png", stacked);

    return 0;
}

void stack(cv::Mat src, cv::Mat lyr, cv::Mat &out) {
    out = src.clone();

    for (int i = 0; i < out.rows; i++) {
        for (int j = 0; j < out.cols; j++) {
            cv::Vec3b& outPixel = out.at<cv::Vec3b>(i, j);
            cv::Vec4b& lyrPixel = lyr.at<cv::Vec4b>(i, j);

            if (lyrPixel[3] != 0) {
                outPixel[0] = lyrPixel[0];
                outPixel[1] = lyrPixel[1];
                outPixel[2] = lyrPixel[2];
            }
        }
    }
}

void makeTransparent(cv::Mat src, cv::Mat &srcTransparent) {
    cv::cvtColor(src, srcTransparent, cv::COLOR_BGR2BGRA);

    for (int i = 0; i < srcTransparent.rows; i++) {
        for (int j = 0; j < srcTransparent.cols; j++) {
            cv::Vec4b& pixel = srcTransparent.at<cv::Vec4b>(i, j);

            if (pixel[0] <= 5 && pixel[1] <= 5 && pixel[2] <= 5) {
                pixel[3] = 0;
            }
        }
    }
}

void circleDetectCrop(cv::Mat src, cv::Mat &srcCropped, std::string path) {
    cv::Mat srcGray;
    cv::cvtColor(src, srcGray, cv::COLOR_BGR2GRAY);
    cv::medianBlur(srcGray, srcGray, 5);
    std::vector<cv::Vec3f> circles;

    HoughCircles(
        srcGray, circles, cv::HOUGH_GRADIENT, 1,
        srcGray.rows / 16,  // Distance between circles
        100, 30, 1, 1500 // Last two: min_radius, max_radius
    );

    cv::Vec3i max = circles[0];
    for(std::size_t i = 1; i < circles.size(); i++)
    {
        cv::Vec3i c = circles[i];

        if (c[2] > max[2]) {
            max = circles[i];
        }
    }

    srcCropped = src(
        cv::Range(
            max[1] - max[2] - 20,
            max[1] + max[2] + 20
        ),
        cv::Range(
            max[0] - max[2] - 20,
            max[0] + max[2] + 20
        )
    );

    writeDetectedCircles(src, circles, path);
}

void writeDetectedCircles(cv::Mat src, std::vector<cv::Vec3f> circles, std::string path) {
    cv::Mat srcCopy = src.clone();

    for(std::size_t i = 0; i < circles.size(); i++)
    {
        cv::Vec3i c = circles[i];
        cv::circle(
            srcCopy,
            cv::Point(c[0], c[1]), // center
            1, // radius
            cv::Scalar(0,100,100),
            3,
            cv::LINE_AA
        );
        cv::circle(
            srcCopy,
            cv::Point(c[0], c[1]), // center
            c[2], // radius
            cv::Scalar(255,0,255),
            3,
            cv::LINE_AA
        );
    }

    cv::imwrite(path + "/detected-circles.png", srcCopy);
}

void getHomography(cv::Mat src, cv::Mat trg, cv::Mat &homography, std::string path) {
    cv::Ptr<cv::SIFT> sift = cv::SIFT::create(
        0, // nfeatures
        3, // nOctaveLayers
        0.02, // contrastThreshold
        50.0, // edgeThreshold
        1.6, // sigma
        false // enable_precise_upscale
    );

    // Detect keypoints and compute descriptors
    std::vector<cv::KeyPoint> srcKeypoints, trgKeypoints;
    cv::Mat srcDescriptors, trgDescriptors;

    sift->detectAndCompute(src, cv::noArray(), srcKeypoints, srcDescriptors);
    sift->detectAndCompute(trg, cv::noArray(), trgKeypoints, trgDescriptors);

    // Match features using FLANN matcher
    cv::Ptr<cv::DescriptorMatcher> matcher =
        cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
    std::vector<std::vector<cv::DMatch>> knn_matches;
    matcher->knnMatch(srcDescriptors, trgDescriptors, knn_matches, 2);

    // Filter matches using Lowe's ratio test
    const float ratio_thresh = 0.7f;
    std::vector<cv::DMatch> good_matches;

    for (size_t i = 0; i < knn_matches.size(); i++) {
        if (knn_matches[i][0].distance < ratio_thresh * knn_matches[i][1].distance) {
            good_matches.push_back(knn_matches[i][0]);
        }
    }

    // Extract location of good matches
    std::vector<cv::Point2f> points1, points2;
    for (const auto& match : good_matches) {
        points1.push_back(srcKeypoints[match.queryIdx].pt);
        points2.push_back(trgKeypoints[match.trainIdx].pt);
    }

    // Calculate homography matrix
    std::vector<uchar> inliers_mask;
    homography = cv::findHomography(points1, points2, cv::RANSAC, 3.0, inliers_mask);

    writeMatchedPoints(
        inliers_mask,
        good_matches,
        src, srcKeypoints,
        trg, trgKeypoints,
        path
    );
}

void writeMatchedPoints(
    std::vector<uchar> inliers_mask,
    std::vector<cv::DMatch> good_matches,
    cv::Mat src, std::vector<cv::KeyPoint> srcKeypoints,
    cv::Mat trg, std::vector<cv::KeyPoint> trgKeypoints,
    std::string path
) {
    // Filter matches using homography mask
    std::vector<cv::DMatch> homography_matches;
    for (size_t i = 0; i < inliers_mask.size(); i++) {
        if (inliers_mask[i]) {
            homography_matches.push_back(good_matches[i]);
        }
    }

    // Draw matches
    cv::Mat img_matches;
    cv::drawMatches(
        src, srcKeypoints, trg, trgKeypoints,
        homography_matches, img_matches
    );
    cv::imwrite(path + "/detected-matches.png", img_matches);
}
