/***************************************************************************
* ALPS++/alea library
*
* alps/alea/simplebinning.h     Monte Carlo observable class
*
* $Id$
*
* Copyright (C) 1994-2002 by Matthias Troyer <troyer@itp.phys.ethz.ch>,
*                            Beat Ammon <ammon@ginnan.issp.u-tokyo.ac.jp>,
*                            Andreas Laeuchli <laeuchli@itp.phys.ethz.ch>,
*                            Synge Todo <wistaria@comp-phys.org>,
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
**************************************************************************/

#ifndef ALPS_ALEA_SIMPLEBINNING_H
#define ALPS_ALEA_SIMPLEBINNING_H

#include <alps/config.h>
#include <alps/alea/observable.h>
#include <alps/alea/simpleobservable.h>
#include <alps/xml.h>

//=======================================================================
// SimpleBinning
//
// simple binning strategy
//-----------------------------------------------------------------------

namespace alps {

template <class T=double>
class SimpleBinning : public AbstractBinning<T>
{
 public:
  typedef T value_type;  
  typedef typename obs_value_traits<T>::time_type time_type;
  typedef typename obs_value_traits<T>::size_type size_type;
  typedef typename obs_value_traits<T>::count_type count_type;
  typedef typename obs_value_traits<T>::result_type result_type;

  static const bool has_tau=true;
  static const int magic_id=2;

  SimpleBinning(uint32_t=0);
  virtual ~SimpleBinning() {}

  void reset(bool forthermalization=false);
  void operator<<(const T& x);
  
  uint32_t count() const {return is_thermalized() ? count_ : 0;} // number of measurements performed
  result_type mean() const;
  result_type variance() const;
  result_type error(uint32_t bin_used=std::numeric_limits<uint32_t>::max()) const;
  time_type tau() const; // autocorrelation time
    
  uint32_t binning_depth() const; 
    // depth of logarithmic binning hierarchy = log2(measurements())
    
  value_type min() const {return min_;}
  value_type max() const {return max_;}

  uint32_t get_thermalization() const { return is_thermalized() ? thermal_count_ : count_;}

  uint32_t size() const { return obs_value_traits<T>::size(min_);}
  
  void output_scalar(std::ostream& out) const;
  void output_vector(std::ostream& out) const;
#ifndef ALPS_WITHOUT_OSIRIS
  virtual void save(ODump& dump) const;
  virtual void load(IDump& dump);
#endif

  std::string evaluation_method() const { return "binning";}

  void write_scalar_xml(std::ostream& xml) const;
  template <class IT> void write_vector_xml(std::ostream& xml, IT) const;

  void write_scalar_xml(oxstream& oxs) const;
  template <class IT> void write_vector_xml(oxstream& oxs, IT) const;
  
private:
  std::vector<result_type> sum_; // sum of measurements in the bin
  std::vector<result_type> sum2_; // sum of the squares
  std::vector<uint32_t> bin_entries_; // number of measurements
  std::vector<result_type> last_bin_; // the last value measured
  
  uint32_t count_; // total number of measurements (=bin_entries_[0])
  uint32_t thermal_count_; // meaurements performed during thermalization
  value_type min_,max_; // minimum and maximum value
  
  // some fast inlined functions without any range checks
  result_type binmean(uint32_t i) const ;
  result_type binvariance(uint32_t i) const;
};

template <class T>
inline SimpleBinning<T>::SimpleBinning(uint32_t)
{
  reset();
}


// Reset the observable. 
template <class T>
inline void SimpleBinning<T>::reset(bool forthermalization)
{
  typedef typename obs_value_traits<T>::count_type count_type;

  AbstractBinning<T>::reset(forthermalization);
  
  thermal_count_= (forthermalization ? count_ : 0);

  sum_.clear();
  sum2_.clear();
  bin_entries_.clear();
  last_bin_.clear();

  count_ = 0;

  min_ =  obs_value_traits<T>::max();
  max_ = -obs_value_traits<T>::max();
}


// add a new measurement
template <class T>
inline void SimpleBinning<T>::operator<<(const T& x) 
{
  typedef typename obs_value_traits<T>::count_type count_type;

  // set sizes if starting additions
  if(count_==0)
  {
    last_bin_.resize(1);
    sum_.resize(1);
    sum2_.resize(1);
    bin_entries_.resize(1);
    obs_value_traits<result_type>::resize_same_as(last_bin_[0],x);
    obs_value_traits<result_type>::resize_same_as(sum_[0],x);
    obs_value_traits<result_type>::resize_same_as(sum2_[0],x);
    obs_value_traits<result_type>::resize_same_as(max_,x);
    obs_value_traits<result_type>::resize_same_as(min_,x);
  }
  
  if(obs_value_traits<T>::size(x)!=size())
    boost::throw_exception(std::runtime_error("Size of argument does not match in SimpleBinning<T>::add"));

  // store x, x^2 and the minimum and maximum value
  last_bin_[0]=obs_value_cast<result_type,value_type>(x);
  sum_[0]+=obs_value_cast<result_type,value_type>(x);
  sum2_[0]+=obs_value_cast<result_type,value_type>(x)*obs_value_cast<result_type,value_type>(x);
  obs_value_traits<T>::check_for_max(x,max_);
  obs_value_traits<T>::check_for_min(x,min_);

  uint32_t i=count_;
  count_++;
  bin_entries_[0]++;
  uint32_t binlen=1;
  std::size_t bin=0;

  // binning
  do 
    {
      if(i&1) 
	{ 
	  // a bin is filled
	  binlen*=2;
	  bin++;
          if(bin>=last_bin_.size())
          {
	    last_bin_.resize(std::max(bin+1,last_bin_.size()));
	    sum_.resize(std::max(bin+1, sum_.size()));
	    sum2_.resize(std::max(bin+1,sum2_.size()));
	    bin_entries_.resize(std::max(bin+1,bin_entries_.size()));

	    obs_value_traits<result_type>::resize_same_as(last_bin_[bin],x);
	    obs_value_traits<result_type>::resize_same_as(sum_[bin],x);
	    obs_value_traits<result_type>::resize_same_as(sum2_[bin],x);
	  }

	  result_type x1=(sum_[0]-sum_[bin]);
	  x1/=count_type(binlen);

	  result_type y1 = x1*x1;

	  last_bin_[bin]=x1;
	  sum2_[bin] += y1;
	  sum_[bin] = sum_[0];
	  bin_entries_[bin]++;
	}
      else
	break;
    } while ( i>>=1);
}


template <class T>
inline uint32_t SimpleBinning<T>::binning_depth() const
{
  return ( int(sum_.size())-5 < 1 ) ? 1 : int(sum_.size())-5;
}


template <class T>
inline typename SimpleBinning<T>::result_type SimpleBinning<T>::binmean(uint32_t i) const 
{
  typedef typename obs_value_traits<T>::count_type count_type;

  return sum_[i]/(count_type(bin_entries_[i]) * count_type(1<<i));
}


template <class T>
inline typename SimpleBinning<T>::result_type SimpleBinning<T>::binvariance(uint32_t i) const 
{
  typedef typename obs_value_traits<T>::count_type count_type;

  result_type retval(sum2_[i]);
  retval/=count_type(bin_entries_[i]);
  retval-=binmean(i)*binmean(i);
  return retval;
}




//---------------------------------------------------------------------
// EVALUATION FUNCTIONS
//---------------------------------------------------------------------

// MEAN
template <class T>  
inline typename SimpleBinning<T>::result_type SimpleBinning<T>::mean() const 
{
  if (count()==0)
     boost::throw_exception(NoMeasurementsError());
  return sum_[0]/count_type(count());
}


// VARIANCE
template <class T>
inline typename SimpleBinning<T>::result_type SimpleBinning<T>::variance() const
{
  typedef typename obs_value_traits<T>::count_type count_type;

  if (count()==0)
     boost::throw_exception(NoMeasurementsError());

  if(count()<2) 
    {
      result_type retval;
      obs_value_traits<T>::resize_same_as(retval,min_);
      retval=obs_value_traits<result_type>::max();
      return retval;
    }

  return (sum2_[0] - sum_[0]*sum_[0]/count_type(count()))/count_type(count()-1);
}


// error estimated from bin i, or from default bin if <0
template <class T>
inline typename SimpleBinning<T>::result_type SimpleBinning<T>::error(uint32_t i) const
{
  typedef typename obs_value_traits<T>::count_type count_type;

  if (count()==0)
     boost::throw_exception(NoMeasurementsError());
 
  if (i==std::numeric_limits<uint32_t>::max()) 
    i=binning_depth()-1;

  if (i > binning_depth()-1)
   boost::throw_exception(std::invalid_argument("invalid bin  in SimpleBinning<T>::error"));
  
  uint32_t binsize_ = bin_entries_[i];
  
  result_type correction = binvariance(i)/binvariance(0);
  using std::sqrt;
  using alps::sqrt;
  return sqrt(correction*variance()/count_type(binsize_-1));
}


template <class T>
inline typename obs_value_traits<T>::time_type SimpleBinning<T>::tau() const
{
  typedef typename obs_value_traits<T>::count_type count_type;

  if (count()==0)
     boost::throw_exception(NoMeasurementsError());

  if( binning_depth() >= 2 )
  {
    count_type factor =count()-1;
    result_type er(error()*error()*factor/variance());
    return 0.5*(er-1.);
  }
  else
  {
    time_type retval;
    obs_value_traits<T>::resize_same_as(retval,min_);
    retval=obs_value_traits<T>::t_max();
    return retval;
  }
}


template <class T>
void SimpleBinning<T>::output_scalar(std::ostream& out) const
{
  if(count())
  {
    out << ": " << mean() << " +/- " << error() << "; tau = "
        << tau() << std::endl;
    if (binning_depth()>1)
    { 
      // detailed errors
      std::ios::fmtflags oldflags = out.setf(std::ios::left,std::ios::adjustfield);
      for(int i=0;i<binning_depth();i++)
        out << "    bin #" << std::setw(3) <<  i+1 
            << " : " << std::setw(8) << count()/(1<<i)
	    << " entries: error = " << error(i) << std::endl;
      out.setf(oldflags);
    }
  }
}

template <class T>
void SimpleBinning<T>::write_scalar_xml(std::ostream& xml) const { 
  for(int i=0;i<binning_depth();i++)
    xml << "\n<BINNED>"
        << "<COUNT>" << count()/(1<<i) << "</COUNT>"
        << "<MEAN method=\"simple\">" << binmean(i) << "</MEAN>"
        << "<ERROR method=\"simple\">" << error(i) << "</ERROR></BINNED>";
}

template <class T> template <class IT> 
void SimpleBinning<T>::write_vector_xml(std::ostream& xml, IT it) const {
  for(int i=0;i<binning_depth();i++)
    xml << "\n<BINNED>"
        << "<COUNT>" << count()/(1<<i) << "</COUNT>"
        << "<MEAN method=\"simple\">" << obs_value_traits<result_type>::slice_value(binmean(i),it) << "</MEAN>"
        << "<ERROR method=\"simple\">" << obs_value_traits<result_type>::slice_value(error(i),it) << "</ERROR></BINNED>";
}

template <class T>
void SimpleBinning<T>::write_scalar_xml(oxstream& oxs) const { 
  for (int i = 0; i < binning_depth(); ++i) {
    oxs << start_tag("BINNED")<< no_linebreak
	<< start_tag("COUNT") << count()/(1<<i) << end_tag
        << start_tag("MEAN") << attribute("method", "simple") << precision(binmean(i), 8) << end_tag
        << start_tag("ERROR") << attribute("method", "simple") << precision(error(i), 3) << end_tag
	<< end_tag("BINNED");
  }
}

template <class T> template <class IT> 
void SimpleBinning<T>::write_vector_xml(oxstream& oxs, IT it) const {
  for (int i = 0; i < binning_depth() ; ++i) {
    oxs << start_tag("BINNED")<< no_linebreak
	<< start_tag("COUNT") << count()/(1<<i) << end_tag
        << start_tag("MEAN") << attribute("method", "simple") << precision(obs_value_traits<result_type>::slice_value(binmean(i),it), 8) << end_tag
        << start_tag("ERROR") << attribute("method", "simple") << precision(obs_value_traits<result_type>::slice_value(error(i),it), 3)	<< end_tag
	<< end_tag("BINNED");
  }
}


template <class T>
inline void SimpleBinning<T>::output_vector(std::ostream& out) const
{
  if(count())
  {
    result_type mean_(mean());
    result_type error_(error());
    time_type tau_(tau());
    std::vector<result_type> errs_(binning_depth(),error_);
    for (int i=0;i<binning_depth();++i)
      errs_[i]=error(i);
      
    out << "\n";
    for (typename obs_value_traits<result_type>::slice_iterator sit=
           obs_value_traits<result_type>::slice_begin(mean_);
          sit!=obs_value_traits<result_type>::slice_end(mean_);++sit)
    {
      out << "Entry[" << obs_value_traits<result_type>::slice_name(mean_,sit) << "]: " 
          << obs_value_traits<result_type>::slice_value(mean_,sit) << " +/- " 
          << obs_value_traits<result_type>::slice_value(error_,sit) << "; tau = "
          << obs_value_traits<time_type>::slice_value(tau_,sit) << std::endl;
      if (binning_depth()>1)
	{
	  // detailed errors
	  std::ios::fmtflags oldflags = out.setf(std::ios::left,std::ios::adjustfield);
	  for(int i=0;i<binning_depth();i++)
	    out << "    bin #" << std::setw(3) <<  i+1 
		<< " : " << std::setw(8) << count()/(1<<i)
		<< " entries: error = "
		<< obs_value_traits<result_type>::slice_value(errs_[i],sit)
		<< std::endl;
	  out.setf(oldflags);
	}
    }
  }
}

#ifndef ALPS_WITHOUT_OSIRIS

template <class T>
inline void SimpleBinning<T>::save(ODump& dump) const
{
  AbstractBinning<T>::save(dump);
  dump << sum_ << sum2_ << bin_entries_ << last_bin_ << count_ << thermal_count_
       << min_ << max_;
}

template <class T>
inline void SimpleBinning<T>::load(IDump& dump) 
{
  AbstractBinning<T>::load(dump);
  dump >> sum_ >> sum2_ >> bin_entries_ >> last_bin_ >> count_ >> thermal_count_
       >> min_ >> max_;
}
#endif

} // end namespace alps

#endif // ALPS_ALEA_SIMPLEBINNING_H
