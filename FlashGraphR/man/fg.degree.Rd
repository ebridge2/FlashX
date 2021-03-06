% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/flashgraph.R
\name{fg.degree}
\alias{fg.degree}
\title{Degree of the vertices in a graph}
\usage{
fg.degree(graph, mode = c("both", "in", "out"))
}
\arguments{
\item{graph}{The FlashGraph object}

\item{mode}{Character string. "out" for out-degree, "in" for in-degree,
"both" for the sum of the two. This argument is ignored for
undirected graphs.}
}
\value{
A numeric vector with the degree of each vertex in the graph.
}
\description{
Get the degree of vertices in a graph.
}
\author{
Da Zheng <dzheng5@jhu.edu>
}
