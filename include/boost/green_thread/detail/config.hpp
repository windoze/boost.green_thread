//
//  config.hpp
//  Boost.GreenThread
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Copyright (c) 2015 Chen Xu

#ifndef BOOST_GREEN_THREAD_DETAIL_CONFIG_H
#define BOOST_GREEN_THREAD_DETAIL_CONFIG_H

#include <boost/config.hpp>

#ifdef BOOST_GREEN_THREAD_DECL
#  undef BOOST_GREEN_THREAD_DECL
#endif

#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_GREEN_THREAD_DYN_LINK)
#  if defined(BOOST_GREEN_THREAD_SOURCE)
#    define BOOST_GREEN_THREAD_DECL BOOST_SYMBOL_EXPORT
#  else
#    define BOOST_GREEN_THREAD_DECL BOOST_SYMBOL_IMPORT
#  endif
#endif

#if ! defined(BOOST_GREEN_THREAD_DECL)
#  define BOOST_GREEN_THREAD_DECL
#endif

//  enable automatic library variant selection  ------------------------------//
#if !defined(BOOST_GREEN_THREAD_SOURCE) && !defined(BOOST_ALL_NO_LIB)
#	define BOOST_LIB_NAME boost_green_thread
#	if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_GREEN_THREAD_DYN_LINK)
#		define BOOST_DYN_LINK
#	endif
#	include <boost/config/auto_link.hpp>
#endif  // auto-linking disabled

#endif // BOOST_GREEN_THREAD_DETAIL_CONFIG_H
