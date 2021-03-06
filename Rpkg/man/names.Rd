% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/FlashR_base.R
\docType{methods}
\name{names}
\alias{names}
\alias{names,fm-method}
\alias{names,fmV-method}
\alias{names<-,fm,fm-method}
\alias{names<-,fmV,fmV-method}
\title{The Names of an Object}
\usage{
\S4method{names}{fm}(x)

\S4method{names}{fmV}(x)

\S4method{names}{fm,fm}(x) <- value

\S4method{names}{fmV,fmV}(x) <- value
}
\arguments{
\item{x}{a FlashR array}

\item{value}{a FlashR array.}
}
\value{
\code{names} returns \code{NULL} or a FlashR array of the same
length as \code{x}. \code{names<-} returns the updated object.
}
\description{
Functions to get or set the names of an object.
}
\details{
Currently, \code{value} has to have the same length as \code{x} and has
the same shape. FlashR currently doesn't support character vectors yet.
}

