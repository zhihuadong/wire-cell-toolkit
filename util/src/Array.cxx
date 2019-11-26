#include "WireCellUtil/Array.h"

#include <unsupported/Eigen/FFT>

#include <algorithm>
#include <complex>

using namespace WireCell;
using namespace WireCell::Array;

thread_local static Eigen::FFT< float > gEigenFFT;

//http://stackoverflow.com/a/33636445

WireCell::Array::array_xxc WireCell::Array::dft(const WireCell::Array::array_xxf& arr)
{
    const int nrows = arr.rows();
    const int ncols = arr.cols();

    Eigen::MatrixXcf matc(nrows, ncols);

    for (int irow = 0; irow < nrows; ++irow) {
        Eigen::VectorXcf fspec(ncols); // frequency spectrum 
	// gEigenFFT wants vectors, also input arr is const
	Eigen::VectorXf tmp = arr.row(irow);
	gEigenFFT.fwd(fspec, tmp);
        matc.row(irow) = fspec;
    }

    for (int icol = 0; icol < ncols; ++icol) {
        Eigen::VectorXcf pspec(nrows); // periodicity spectrum
        gEigenFFT.fwd(pspec, matc.col(icol));
        matc.col(icol) = pspec;
    }

    return matc;
}

WireCell::Array::array_xxc WireCell::Array::dft_rc(const WireCell::Array::array_xxf& arr, int dim)
{
    const int nrows = arr.rows();
    const int ncols = arr.cols();

    Eigen::MatrixXcf matc(nrows, ncols);

    if (dim == 0) {
        for (int irow = 0; irow < nrows; ++irow) {
            Eigen::VectorXcf fspec(ncols);
            Eigen::VectorXf tmp = arr.row(irow);
            gEigenFFT.fwd(fspec, tmp);
            matc.row(irow) = fspec;
        }
    }
    else if (dim == 1) {
        for (int icol = 0; icol < ncols; ++icol) {
            Eigen::VectorXcf fspec(nrows);
            Eigen::VectorXf tmp = arr.col(icol);
            gEigenFFT.fwd(fspec, tmp);
            matc.col(icol) = fspec;
        }
    }        
    return matc;
}

WireCell::Array::array_xxc WireCell::Array::dft_cc(const WireCell::Array::array_xxc& arr, int dim)
{
    const int nrows = arr.rows();
    const int ncols = arr.cols();

    Eigen::MatrixXcf matc(nrows, ncols);

    matc = arr.matrix();
    
    if (dim == 0) {
        for (int irow = 0; irow < nrows; ++irow) {
            Eigen::VectorXcf pspec(ncols);
            gEigenFFT.fwd(pspec,matc.row(irow));
            matc.row(irow) = pspec;
        }
    }
    else {
        for (int icol = 0; icol < ncols; ++icol) {
            Eigen::VectorXcf pspec(nrows);
            gEigenFFT.fwd(pspec, matc.col(icol));
            matc.col(icol) = pspec;
        }
    }
    return matc;
}



WireCell::Array::array_xxf WireCell::Array::idft(const WireCell::Array::array_xxc& arr)
{
    const int nrows = arr.rows();
    const int ncols = arr.cols();

    // gEigenFFT works on matrices, not arrays, also don't step on const input
    Eigen::MatrixXcf partial(nrows, ncols);
    partial = arr.matrix();

    for (int icol = 0; icol < ncols; ++icol) {
        Eigen::VectorXcf pspec(nrows); // wire spectrum
        gEigenFFT.inv(pspec, partial.col(icol));
        partial.col(icol) = pspec;
    }

    //shared_array_xxf ret = std::make_shared<array_xxf> (nrows, ncols);
    array_xxf ret(nrows, ncols);

    for (int irow = 0; irow < nrows; ++irow) {
        Eigen::VectorXf wave(ncols); // back to real-valued time series
        gEigenFFT.inv(wave, partial.row(irow));
        ret.row(irow) = wave;
    }

    return ret;
}

WireCell::Array::array_xxc WireCell::Array::idft_cc(const WireCell::Array::array_xxc& arr, int dim)
{
    const int nrows = arr.rows();
    const int ncols = arr.cols();

    // gEigenFFT works on matrices, not arrays, also don't step on const input
    Eigen::MatrixXcf ret(nrows, ncols);
    ret = arr.matrix();

    if (dim == 1) {
        for (int icol = 0; icol < ncols; ++icol) {
            Eigen::VectorXcf pspec(nrows);
            gEigenFFT.inv(pspec, ret.col(icol));
            ret.col(icol) = pspec;
        }
    }
    else if (dim == 0) {
        for (int irow = 0; irow < nrows; ++irow) {
            Eigen::VectorXcf pspec(ncols);
            gEigenFFT.inv(pspec, ret.row(irow));
            ret.row(irow) = pspec;
        }
    }
    return ret;
}

WireCell::Array::array_xxf WireCell::Array::idft_cr(const WireCell::Array::array_xxc& arr, int dim)
{
    const int nrows = arr.rows();
    const int ncols = arr.cols();

    // gEigenFFT works on matrices, not arrays, also don't step on const input
    Eigen::MatrixXcf partial(nrows, ncols);
    partial = arr.matrix();

    array_xxf ret(nrows, ncols);

    if (dim == 0) {
        for (int irow = 0; irow < nrows; ++irow) {
            Eigen::VectorXf wave(ncols); // back to real-valued time series
            gEigenFFT.inv(wave, partial.row(irow));
            ret.row(irow) = wave;
        }
    }
    else if (dim == 1) {
        for (int icol = 0; icol < ncols; ++icol) {
            Eigen::VectorXf wave(nrows);
            gEigenFFT.inv(wave, partial.col(icol));
            ret.col(icol) = wave;
        }
    }
    return ret;
}



// this is a cut-and-paste mashup of dft() and idft() in order to avoid temporaries.
WireCell::Array::array_xxf
WireCell::Array::deconv(const WireCell::Array::array_xxf& arr,
			const WireCell::Array::array_xxc& filter)
{
    const int nrows = arr.rows();
    const int ncols = arr.cols();

    Eigen::MatrixXcf matc(nrows, ncols);
    for (int irow = 0; irow < nrows; ++irow) {
	Eigen::VectorXcf fspec(ncols); // frequency spectrum 
	// gEigenFFT wants vectors, also input arr is const
	Eigen::VectorXf tmp = arr.row(irow);
	gEigenFFT.fwd(fspec, tmp);
	matc.row(irow) = fspec;
    }
    
    for (int icol = 0; icol < ncols; ++icol) {
	Eigen::VectorXcf pspec(nrows); // periodicity spectrum
	gEigenFFT.fwd(pspec, matc.col(icol));
	matc.col(icol) = pspec;
    }

    // deconvolution via multiplication in frequency space
    Eigen::MatrixXcf filt = matc.array() * filter;

    for (int icol = 0; icol < ncols; ++icol) {
        Eigen::VectorXcf pspec(nrows); // wire spectrum
        gEigenFFT.inv(pspec, filt.col(icol));
        filt.col(icol) = pspec;
    }

    array_xxf ret(nrows, ncols);

    for (int irow = 0; irow < nrows; ++irow) {
        Eigen::VectorXf wave(ncols); // back to real-valued time series
        gEigenFFT.inv(wave, filt.row(irow));
        ret.row(irow) = wave;
    }

    return ret;
}

