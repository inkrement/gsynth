\name{plot.gsynth}
\alias{plot.gsynth}
\title{Plotting}
\description{Visualizes estimation results of the generalized synthetic control method.}
\usage{\method{plot}{gsynth}(x, type = "gap", xlim = NULL, ylim = NULL,
            xlab = NULL, ylab = NULL, legendOff = FALSE, raw = "none",
            main = NULL, nfactors = NULL, id = NULL, axis.adjust = FALSE, \dots)
}
\arguments{
  \item{x}{a \code{\link{gsynth}} object.}
  \item{type}{a string that specifies the type of the plot. Must be one
    of the following: "gap" (plotting the average treatment effect on
    the treated; "raw" (plotting the raw data); "counterfactual"    
    (plotting predicted Y(0)'s); "factors" (plotting estimated factors);    
    "loadings" (plotting the distribution of estimated factor
    loadings); "missing" (plotting status of each unit at each time point).}
  \item{xlim}{a two-element numeric vector specifying the range of x-axis. When 
    class of time varible is string, must specify not original value but a counting number e.g. \code{xlim=c(1,30)}.}
  \item{ylim}{a two-element numeric vector specifying the range of y-axis.}
  \item{xlab}{a string indicating the label of the x-axis.}
  \item{ylab}{a string indicating the label of the y-axis.}
  \item{legendOff}{a logical flag controlling whether to show the legend.}
  \item{raw}{a string indicating whether or how raw data for the outcome
    variable will be shown
    in the "counterfactual" plot. Ignored if \code{type} is not
    "counterfactual". Must be one of the following: "none" (not showing
    the raw data); "band" (showing the middle 90 percentiles of the raw
    data); and "all" (showing the raw data as they are).}
  \item{main}{a string that controls the title of the plot. If not
    supplied, no title will be shown.}
  \item{nfactors}{a positive integer that specifies the number of
    factors to be shown. The maximum number if 4. Ignored if \code{type}
  is not "factors"}
  \item{id}{a unit identifier of which the predicted counterfactual or
    the difference between actual and predicted counterfactual is to be shown. 
    It can also be a vector specifying units to be plotted if \code{type=="missing"} when data magitude is large. Ignored if \code{type} is none of 
    "missing", "counterfactual", "gap".}
  \item{axis.adjust}{a logic flag indicating whether to adjust labels on x-axis. 
    Useful when class of time variable is string and data magnitude is large.}
  \item{\dots}{other argv.}
}
\details{
  \code{plot.gsynth} visualizes the raw data used by, or estimation
  results obtained from, the generalized synthetic control method.
}
\author{
  Yiqing Xu <yiqingxu@ucsd.edu>
  
  Licheng Liu <liulch.16@sem.tsinghua.edu.cn>
}
\references{Yiqing Xu. 2017. "Generalized Synthetic Control Method: Causal Inference
  with Interactive Fixed Effects Models." Political Analysis, Vol. 25, 
  Iss. 1, January 2017, pp. 57-76. Available at: \url{https://doi.org/10.1017/pan.2016.2}.

  See \url{http://yiqingxu.org/software/gsynth/gsynth_examples.html} for
  more detailed information.
}
\seealso{
  \code{\link{gsynth}} and \code{\link{print.gsynth}}
}



