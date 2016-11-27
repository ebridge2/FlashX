% Generated by roxygen2 (4.1.1): do not edit by hand
% Please edit documentation in R/FlashR.R
\name{materialize}
\alias{fm.materialize}
\alias{fm.materialize.list}
\alias{fm.set.materialize.level}
\alias{materialize}
\title{Materialize virtual FlashMatrix objects.}
\usage{
fm.materialize.list(args)

fm.materialize(...)

fm.set.materialize.level(fm, level, in.mem = fm.in.mem(fm))
}
\arguments{
\item{args}{a list of virtual FlashMatrix objects.}

\item{...}{a list of virtual FlashMatrix objects.}

\item{fm}{a FlashMatrix object.}

\item{level}{an integer. 1 means not saving computation results, 2 means
saving computation results.}

\item{in.mem}{a logical value, indicating whether to save the computation
results in memory.}
}
\value{
a list of materialized compuation results.
}
\description{
FlashMatrix lazily evaluates many operations and outputs virtual objects
that represent computation results. \code{fm.materialize.list} and
\code{fm.materialize} explicitly materialize the virtualized computation
and save the computation results to memory or disks. Materialization of
these virtualized computations triggers materialization of other virtualized
computation. By default, FlashMatrix only saves the computation results
specified by the arguments of \code{fm.materialize.list} and
\code{fm.materialize}. However, by setting the materialization level of
a virtualized computation with \code{fm.set.materialize.level}, even if
the virtualized computation is not specified in \code{fm.materialize.list}
and \code{fm.materialize}, its computation result can still be saved.
}
