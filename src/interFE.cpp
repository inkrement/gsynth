# include <RcppArmadillo.h>
// [[Rcpp::depends(RcppArmadillo)]]
// [[Rcpp::plugins(cpp11)]]

using namespace Rcpp ;

/* ******************* Useful Functions  *********************** */

/* cross product */
arma::mat crossprod (arma::mat x, arma::mat y) {
  return(x.t() * y);
}

/* Expectation :E if Iij==0, Eij=FEij */
arma::mat E_adj (arma::mat E, arma::mat FE,
                 arma::mat I) {
  int T = E.n_rows ;
  int N = E.n_cols ;
  arma::mat EE = E ;
  for (int i = 0; i < T; i++) {
    for (int j = 0; j < N; j++) {
      if (I(i, j) == 0) {
        EE(i, j) = FE(i, j) ;
      }
    }
  }
  return(EE) ;
}

/* reset FEij=0 if Iij==0 , for residuals or IC*/
arma::mat FE_adj (arma::mat FE, arma::mat I) {
  int T = FE.n_rows ;
  int N = FE.n_cols ;
  arma::mat FEE = FE ;
  for (int i = 0; i < T; i++) {
    for (int j = 0; j < N; j++) {
      if (I(i, j) == 0) {
        FEE(i, j) = 0 ;
      }
    }
  }
  return(FEE) ;
}

/* drop values if Iij == 1 */
arma::mat FE_missing (arma::mat FE, arma::mat I) {
  int T = FE.n_rows ;
  int N = FE.n_cols ;
  arma::mat FEE = FE ;
  for (int i = 0; i < T; i++) {
    for (int j = 0; j < N; j++) {
      if (I(i, j) == 1) {
        FEE(i, j) = 0 ;
      }
    }
  }
  return(FEE) ;
}

/* adjust unbalanced data */
// [[Rcpp::export]]
arma::mat data_ub_adj (arma::mat I_data, arma::mat data) {
  int count = I_data.n_rows ;
  //int total = data.n_rows ;
  int nov = data.n_cols ;
  arma::mat data_adj(count,nov) ;
  data_adj.fill(arma::datum::nan) ;
  int j = 0;
  for(int i=0; i<count; i++){
    if(I_data(i,0)==1){
      data_adj.row(i) = data.row(j);
      j++;
    }
  }
  return(data_adj);
}

/* Three dimensional matrix inverse */
// [[Rcpp::export]]
arma::mat XXinv (arma::cube X) { 
  int p = X.n_slices ;
  arma::mat xx(p, p, arma::fill::zeros) ;
  for (int k = 0; k < p; k++) {
    for (int m = 0; m < p; m++) {
      if (k > m) {
        xx(k, m) = xx(m, k);
      }
      else {
        xx(k, m) = trace(crossprod(X.slice(k), X.slice(m))) ;
      }
    }
  } 
  return(inv(xx)) ;
}

/* unbalanced panel: response demean function */
// [[Rcpp::export]]
List Y_demean (arma::mat Y, int force) {
  int T = Y.n_rows ;
  int N = Y.n_cols ;
  double mu_Y = 0 ;
  arma::mat alpha_Y(N, 1, arma::fill::zeros) ; 
  arma::mat xi_Y(T, 1, arma::fill::zeros) ;
  arma::mat YY = Y ;

  mu_Y  =  accu(YY)/(N*T) ;
  if (force == 0) {
    YY = YY - mu_Y ;
  } 
  
  /* unit fixed effects */
  if (force == 1) {
    alpha_Y  =  mean(YY, 0).t() ; // colMeans, (N * 1) matrix
    YY  =  YY - repmat(alpha_Y.t(), T, 1) ; // (T * N) matrix  
  }
  
  /* time fixed effects  */
  if ( force == 2) {
    xi_Y  =  mean(YY, 1) ; //rowMeans, (N * 1) matrix
    YY  =  YY - repmat(xi_Y, 1, N);  
  }

  if (force == 3) {
    alpha_Y  =  mean(YY, 0).t() ;
    xi_Y  =  mean(YY, 1) ;
    YY  =  YY - repmat(alpha_Y.t(), T, 1) - repmat(xi_Y, 1, N) + mu_Y ;
  }

  List result ;
  result["mu_Y"] = mu_Y ;
  result["YY"] = YY ;
  if (force==1 || force==3) {
    result["alpha_Y"] = alpha_Y ;
  }
  if (force==2 || force==3) {
    result["xi_Y"] = xi_Y ;  
  }

  return(result) ;
}

/* estimate additive fe for unbalanced panel */
// [[Rcpp::export]]
List fe_add (arma::mat alpha_X,
             arma::mat xi_X,
             arma::mat mu_X,
             arma::mat alpha_Y,
             arma::mat xi_Y,
             double mu_Y,
             arma::mat beta,
             int T,
             int N,
             int p,
             int force) {
  
  arma::mat FE_ad(T, N, arma::fill::zeros) ;
  double mu = 0 ;
  arma::mat alpha(N, 1, arma::fill::zeros) ;
  arma::mat xi(T, 1, arma::fill::zeros) ;

  mu  =  mu_Y - crossprod(mu_X, beta)(0,0) ;
  if (force ==1 || force == 3) {
    alpha  =  alpha_Y - alpha_X * beta - mu ; 
  }
  if (force == 2 || force == 3) {
    xi  =  xi_Y - xi_X * beta - mu ;
  } 

  FE_ad = FE_ad + mu ;

  if (force ==1 || force == 3) {
    FE_ad = FE_ad + repmat(alpha.t(), T, 1) ;
  }
  if (force == 2 || force == 3) {
    FE_ad = FE_ad + repmat(xi, 1, N) ;
  }

  List result ;
  result["mu"] = mu ;
  result["FE_ad"] = FE_ad ;
  if (force ==1 || force == 3) {
    result["alpha"] = alpha ;
  }
  if (force == 2 || force == 3) {
    result["xi"] = xi ;
  }

  return(result) ;
}

/* estimate additive fe for unbalanced panel, without covariates */
// [[Rcpp::export]]
List fe_add2 (arma::mat alpha_Y,
              arma::mat xi_Y,
              double mu_Y,
              int T,
              int N,
              int force) {
  arma::mat FE_ad(T, N, arma::fill::zeros) ;
  double mu = 0 ;
  arma::mat alpha(N, 1, arma::fill::zeros) ;
  arma::mat xi(T, 1, arma::fill::zeros) ;

  mu =  mu_Y;
  if (force ==1 || force ==3) {
    alpha =  alpha_Y - mu_Y ;
  }
  if (force == 2 || force == 3) {
    xi =  xi_Y - mu_Y ;
  }   

  FE_ad = FE_ad + mu ;

  if (force ==1 || force == 3) {
    FE_ad = FE_ad + repmat(alpha.t(), T, 1) ;
  }
  if (force == 2 || force == 3) {
    FE_ad = FE_ad + repmat(xi, 1, N) ;
  }

  List result ;
  result["mu"] = mu ;
  result["FE_ad"] = FE_ad ;
  if (force ==1 || force == 3) {
    result["alpha"] = alpha ;
  }
  if (force == 2 || force == 3) {
    result["xi"] = xi ;
  }

  return(result) ;
}



/* ******************* Subsidiary Functions  *********************** */

/* Obtain OLS panel estimate */
// [[Rcpp::export]]
arma::mat panel_est (arma::cube X, arma::mat Y, arma::mat MF) {
  int p = X.n_slices ;
  arma::mat xx(p, p, arma::fill::zeros);
  arma::mat xy(p, 1, arma::fill::zeros);
  if (p==1) {
    xx(0,0) = trace(X.slice(0).t() * MF * X.slice(0)) ;
    xy(0,0) = trace(X.slice(0).t() * MF * Y) ;
  }
  if (p>1) {
    for (int k = 0; k < p; k++) {
      arma::mat MFX1 = MF * X.slice(k) ;
      xy(k, 0) = trace(crossprod(MFX1, Y)) ;
      for (int m = k; m < p; m++) {
        arma::mat MFX2 = MF * X.slice(m) ;
        xx(k, m) = trace(crossprod(MFX1, MFX2)) ;
        if (k < m) {
          xx(m, k) = xx(k, m) ;
        }
      }
    }
  }
  return(xx.i() * xy) ;
}

/* Obtain beta given interactive fe */
// [[Rcpp::export]]
arma::mat panel_beta (arma::cube X, arma::mat xxinv,
                      arma::mat Y, arma::mat FE) {
  int p = X.n_slices ; 
  arma::mat xy(p, 1, arma::fill::zeros) ;
  for (int k = 0; k < p; k++) {
    xy(k) = trace(crossprod(X.slice(k), (Y - FE))) ;
  }
  return(xxinv * xy);
}

/* Obtain factors and loading given error */
// [[Rcpp::export]]
List panel_factor (arma::mat E, int r) {
  int T = E.n_rows ;
  int N = E.n_cols ;
  arma::mat factor(T, r, arma::fill::zeros) ;
  arma::mat lambda(N, r, arma::fill::zeros) ;
  arma::mat FE (T, N, arma::fill::zeros) ;
  arma::mat VNT(r, r, arma::fill::zeros) ;
  arma::mat U ;
  arma::vec s ;
  arma::mat V ; 
  if (T < N) { 
    arma::mat EE = E * E.t() /(N * T) ;
    arma::svd( U, s, V, EE) ;
    factor = U.head_cols(r) * sqrt(double(T)) ;
    lambda = E.t() * factor/T ;
    VNT = diagmat(s.head_rows(r)) ;
  } 
  else {
    arma::mat EE = E.t() * E / (N * T) ;
    svd(U, s, V, EE) ;
    lambda = U.head_cols(r) * sqrt(double(N)) ;
    factor = E * lambda / N ;
    VNT = diagmat(s.head_rows(r)) ;
  }
  FE = factor * lambda.t() ;
  List result ;
  result["lambda"] = lambda ;
  result["factor"] = factor ;
  result["VNT"] = VNT ;
  result["FE"] = FE ;
  return(result) ;
  
}

/* Obtain factors and loading given error for ub data,
   useless under the assumption of non-zero grandmean */
// [[Rcpp::export]]
List panel_factor_ub (arma::mat E, arma::mat I, int r, double tolerate) {
  int T = E.n_rows ;
  int N = E.n_cols ;
  int niter = 0;
  double dif = 1.0 ;
  arma::mat F(T, r, arma::fill::zeros) ;
  arma::mat F_old(N, r, arma::fill::zeros) ;
  arma::mat L(N, r, arma::fill::zeros) ;
  arma::mat L_old(N, r, arma::fill::zeros) ;
  arma::mat FE_0(T, N, arma::fill::zeros) ; // intermediate value
  arma::mat FE(T, N, arma::fill::zeros) ;
  arma::mat E_use(T, N, arma::fill::zeros) ; // intermediate value
  arma::mat VNT(r, r, arma::fill::zeros) ;

  List pf = panel_factor(E, r)  ;
  F = as<arma::mat>(pf["factor"]) ;
  L = as<arma::mat>(pf["lambda"]) ;

  while ( (niter<500) && (dif>tolerate) ) {
    niter++ ;
    FE_0 = F * L.t() ; 
    E_use = E_adj(E, FE_0, I) ; // e-step
    pf = panel_factor(E_use, r)  ; // m-step
    F = as<arma::mat>(pf["factor"]) ;
    L = as<arma::mat>(pf["lambda"]) ;
    VNT = as<arma::mat>(pf["VNT"]) ;
    if (T<N) { // factor : projection matrix
      dif = arma::norm(F - F_old, "fro")/(r*T) ;
      F_old = F ;
    } else { // lambda : projection matrix
      dif = arma::norm(L - L_old, "fro")/(r*N) ;
      L_old = L ;      
    }
  }
  FE = FE_adj(F*L.t(), I) ;

  List result ;
  result["niter"] = niter ;
  result["lambda"] = L ;
  result["factor"] = F ;
  result["VNT"] = VNT ;
  result["FE"] = FE ;
  return(result) ;
}

/* Obtain interactive fe directly */
// [[Rcpp::export]]
arma::mat panel_FE (arma::mat E, double lambda) {
  int T = E.n_rows ;
  int N = E.n_cols ;
  int r = T ;
  if (T >= N) {
    r = N ;
  }

  arma::mat FE (T, N, arma::fill::zeros) ;
  arma::mat D(r, r, arma::fill::zeros) ;
  arma::mat U ;
  arma::vec s ;
  arma::mat V ;
  arma::svd( U, s, V, E) ;
  
  for (int i = 0; i < r; i++) {
    if (s(i) > lambda) {
      D(i, i) = s(i) - lambda ;
    } else {
      D(i, i) = 0 ;
    }
  }
  if (T >= N) {
    arma::mat UU = U.cols(0, r-1) ; 
    FE = UU * D * V.t() ;
  }
  else {
    arma::mat VV = V.cols(0, r-1) ;
    FE = U * D * VV.t() ;
  }
  return(FE) ;
}

/* Obtain interactive fe directly: matrix completion,
   useless under the assumption of non-zero grandmean */
// [[Rcpp::export]]
List panel_FE_ub (arma::mat E, arma::mat I, // I: indicator matrix
                double lambda, double tolerate) {
  int T = E.n_rows ;
  int N = E.n_cols ;
  //int r = T ;
  //if (T > N) {
  //  r = N ;
  //}
  double dif = 1.0 ;
  int niter = 0 ;

  arma::mat FE_m (T, N, arma::fill::zeros) ; // the missing matrix
  arma::mat FE (T, N, arma::fill::zeros) ;
  arma::mat FE_old = E ;
  
  // arma::mat EE = E; // replicate data
  while ((dif > tolerate) && (niter < 500)) {
    niter++ ;
    FE = panel_FE(E + FE_m, lambda) ;
    FE_m = FE_missing(FE, I) ;
    dif = arma::norm(FE - FE_old, "fro")/(N*T) ;
    FE_old = FE ;
  }
  List out ;
  out["niter"] = niter ;
  out["FE"] = FE ;
  return(out) ;
}


/* Obtain additive fe for ub data; assume r=0, without covar */
// [[Rcpp::export]]
List fe_ad_iter (arma::mat Y,
                 arma::mat I,
                 int force,
                 double tolerate) {
  
  int T = Y.n_rows ;
  int N = Y.n_cols ;
  double mu = 0 ;
  double mu_old = 0 ;
  double dif = 1.0 ;
  int niter = 0 ;

  arma::mat fit(T, N, arma::fill::zeros) ;
  arma::mat alpha(N, 1, arma::fill::zeros) ;
  arma::mat xi(T, 1, arma::fill::zeros) ;
  arma::mat alpha_old(N, 1, arma::fill::zeros) ;
  arma::mat xi_old(T, 1, arma::fill::zeros) ;
  double mu_Y = 0 ;
  arma::mat alpha_Y(N, 1, arma::fill::zeros) ;
  arma::mat xi_Y(T, 1, arma::fill::zeros) ;

  arma::mat YY = Y ;

  List Y_ad ;
  List Y_fe_ad ;

  while (dif > tolerate && niter <= 500) {

    YY =  E_adj (Y, fit, I) ; // e step: expeactation

    Y_ad = Y_demean(YY, force) ;
    
    mu_Y = as<double>(Y_ad["mu_Y"]) ;
    if (force==1||force==3) {     
      alpha_Y = as<arma::mat>(Y_ad["alpha_Y"]) ;
    }
    if (force==2||force==3) {
      xi_Y = as<arma::mat>(Y_ad["xi_Y"]) ;
    }
    Y_fe_ad = fe_add2(alpha_Y, xi_Y, mu_Y, T, N, force) ; // m step: estimate fe
    
    fit = as<arma::mat>(Y_fe_ad["FE_ad"]) ;

    mu = as<double>(Y_fe_ad["mu"]) ;
    if (force == 0) {
      dif = mu - mu_old ;
      mu_old = mu ;
    }
    if ( force == 1 || force == 3 ) {
      alpha = as<arma::mat>(Y_fe_ad["alpha"]) ;
      dif = arma::norm(alpha - alpha_old, "fro")/N ;
      alpha_old = alpha ; 
    }
    if ( force == 2 ) {
      xi = as<arma::mat>(Y_fe_ad["xi"]) ;
      dif = arma::norm(xi - xi_old, "fro")/T ;
      xi_old = xi ;
    }

    niter = niter + 1 ;
  }

  arma::mat e = YY - fit ;
  e = FE_adj(e, I) ;

  List result;
  result["mu"] = as<double>(Y_fe_ad["mu"]) ;
  result["fit"] = fit ;
  result["niter"] = niter ;
  result["e"] = e ;

  if (force==1||force==3) {
    alpha = as<arma::mat>(Y_fe_ad["alpha"]) ;
    result["alpha"] = alpha ;
  }
  if (force==2||force==3) {
    xi = as<arma::mat>(Y_fe_ad["xi"]) ;
    result["xi"] = xi ;
  }
  return(result) ;
}


/* Obtain additive fe for ub data; assume r=0, with covariates */
// [[Rcpp::export]]
List fe_ad_covar_iter (arma::cube XX,
                       arma::mat xxinv,
                       arma::mat alpha_X,
                       arma::mat xi_X,
                       arma::mat mu_X,
                       arma::mat Y,
                       arma::mat I,
                       int force,
                       double tolerate) {
  int T = Y.n_rows ;
  int N = Y.n_cols ;
  int p = XX.n_slices ;
  double dif = 1.0 ;
  int niter = 0 ;

  arma::mat fit(T, N, arma::fill::zeros) ;
  arma::mat alpha(N, 1, arma::fill::zeros) ;
  arma::mat xi(T, 1, arma::fill::zeros) ;
  double mu_Y = 0 ;
  arma::mat alpha_Y(N, 1, arma::fill::zeros) ;
  arma::mat xi_Y(T, 1, arma::fill::zeros) ;

  arma::mat YY = Y ;
  arma::mat YY_demean(T, N, arma::fill::zeros) ;
  arma::mat beta(p, 1, arma::fill::zeros) ;
  arma::mat beta_old = beta ;

  List Y_ad ;
  List Y_fe_ad ;

  while (dif > tolerate && niter <= 500) {

    YY =  E_adj (Y, fit, I) ; // e-step: expectation
    Y_ad = Y_demean(YY, force) ;
    
    YY_demean = as<arma::mat>(Y_ad["YY"]) ;
    beta = panel_beta(XX, xxinv, YY_demean, arma::zeros<arma::mat>(T,N)) ; // m-step 
    
    mu_Y = as<double>(Y_ad["mu_Y"]) ;
    if (force==1||force==3) {     
      alpha_Y = as<arma::mat>(Y_ad["alpha_Y"]) ;
    }
    if (force==2||force==3) {
      xi_Y = as<arma::mat>(Y_ad["xi_Y"]) ;
    }
    Y_fe_ad = fe_add(alpha_X, xi_X, mu_X, alpha_Y, xi_Y, mu_Y, 
                     beta, T, N, p, force) ;
    // fitted values
    fit.zeros() ;
    for (int i =0; i < p; i++) {
        fit = fit + XX.slice(i) * beta(i) ;
    }
    if (force == 0) {
      fit = fit + mu_Y ;
    }
    if (force == 1) {
      fit = fit + repmat(alpha_Y.t(), T, 1) ;
    }
    if (force == 2) {
      fit = fit + repmat(xi_Y, 1, N) ;
    }
    if (force == 3) {
      fit = fit + repmat(alpha_Y.t(), T, 1) + repmat(xi_Y, 1, N) - mu_Y ;
    }
    
    dif = arma::norm(beta - beta_old, "fro")/p ;
    beta_old = beta ;

    niter = niter + 1 ;
  }

  arma::mat e = YY - fit ;
  e = FE_adj(e, I) ;

  List result;
  result["mu"] = as<double>(Y_fe_ad["mu"]) ;
  result["fit"] = fit ;
  result["niter"] = niter ;
  result["e"] = e ;
  if (p>0) {
    result["beta"] = beta ;
  }
  if (force==1||force==3) {
    alpha = as<arma::mat>(Y_fe_ad["alpha"]) ;
    result["alpha"] = alpha ;
  }
  if (force==2||force==3) {
    xi = as<arma::mat>(Y_fe_ad["xi"]) ;
    result["xi"] = xi ;
  }
  return(result) ;
}

/* Obtain additive fe for ub data; assume r>0 but p=0*/
// [[Rcpp::export]]
List fe_ad_inter_iter (arma::mat Y,
                       arma::mat I,
                       int force,
                       int mc, // whether pac or mc method
                       int r,
                       double lambda,
                       double tolerate
                       ) {
  int T = Y.n_rows ;
  int N = Y.n_cols ;
  double mu = 0 ;
  // double mu_old = 0 ;
  double dif = 1.0 ;
  int niter = 0 ;
  int validF = 1 ; // whether has a factor structure

  arma::mat VNT(r, r) ;
  arma::mat FE_add_use(T, N, arma::fill::zeros) ;
  arma::mat FE_inter_use(T, N, arma::fill::zeros) ;
  arma::mat FE_inter_use_old(T, N, arma::fill::zeros) ;
  arma::mat fit(T, N, arma::fill::zeros) ;
  arma::mat U ;
  
  arma::mat alpha(N, 1, arma::fill::zeros) ;
  arma::mat xi(T, 1, arma::fill::zeros) ;
  double mu_Y = 0 ;
  arma::mat alpha_Y(N, 1, arma::fill::zeros) ;
  arma::mat xi_Y(T, 1, arma::fill::zeros) ;

  arma::mat F(T, r, arma::fill::zeros) ;
  arma::mat L(N, r, arma::fill::zeros) ;

  //arma::mat F_old(T, r, arma::fill::zeros) ;
  //arma::mat L_old(N, r, arma::fill::zeros) ;

  arma::mat YY = Y ;

  List Y_ad ;
  List Y_fe_ad ;
  List pf ;

  while (dif > tolerate && niter <= 500) {
    YY = E_adj(Y, fit, I) ; // e-step: expectation
    // m1: estimate additive fe
    Y_ad = Y_demean(YY - FE_inter_use, force) ;
    mu_Y = as<double>(Y_ad["mu_Y"]) ;
    if (force==1||force==3) {
      alpha_Y = as<arma::mat>(Y_ad["alpha_Y"]) ;
    }
    if (force==2||force==3) {
      xi_Y = as<arma::mat>(Y_ad["xi_Y"]) ;
    }
    Y_fe_ad = fe_add2(alpha_Y, xi_Y, mu_Y,
                      T, N, force) ;
    FE_add_use = as<arma::mat>(Y_fe_ad["FE_ad"]) ; // additive fe
    
    // m2: estimate interactive fe
    U = YY - FE_add_use ;
    if (mc == 0) {
      pf = panel_factor(U, r)  ;
      F = as<arma::mat>(pf["factor"]) ;
      L = as<arma::mat>(pf["lambda"]) ;
      VNT = as<arma::mat>(pf["VNT"]) ;
      FE_inter_use = F * L.t() ; // interactive fe
    }
    else {
      FE_inter_use = panel_FE(U, lambda) ;
    }

    fit = FE_add_use + FE_inter_use ; // new overall fe

    mu = as<double>(Y_fe_ad["mu"]) ;
    //dif = abs(mu - mu_old) ;
    //mu_old = mu ;
    //if (T < N) {
    //  dif = arma::norm(F - F_old, "fro")/(T*r) ;
    //  F_old = F ;
    //} else {
    //  dif = arma::norm(L - L_old, "fro")/(N*r) ;
    //  L_old = L ;
    //}
    dif = arma::norm(FE_inter_use - FE_inter_use_old, "fro")/(N*T) ;
    FE_inter_use_old = FE_inter_use ;

    niter = niter + 1 ;
  }
  arma::mat e = YY - fit ;
  e = FE_adj(e, I) ;

  if (arma::accu(abs(FE_inter_use)) < 1e-10) {
    validF = 0 ;
  }

  List result;
  result["mu"] = mu ;
  result["niter"] = niter ;
  result["fit"] = fit ;
  result["e"] = e ;
  result["validF"] = validF ;
  if (force==1||force==3) {
    alpha = as<arma::mat>(Y_fe_ad["alpha"]) ;
    result["alpha"] = alpha ;
  }
  if (force==2||force==3) {
    xi = as<arma::mat>(Y_fe_ad["xi"]) ;
    result["xi"] = xi ;
  }
  if (mc == 0) {
    result["lambda"] = L ;
    result["factor"] = F ;
    result["VNT"] = VNT ;
  }
  return(result) ;
}

/* Obtain additive fe for ub data; assume r>0 p>0*/
// [[Rcpp::export]]
List fe_ad_inter_covar_iter (arma::cube XX,
                             arma::mat xxinv,
                             arma::mat alpha_X,
                             arma::mat xi_X,
                             arma::mat mu_X,
                             arma::mat Y,
                             arma::mat I,
                             int force,
                             int mc, // whether pac or mc method
                             int r,
                             double lambda,
                             double tolerate
                             ) {
  int T = Y.n_rows ;
  int N = Y.n_cols ;
  int p = XX.n_slices ;
  double dif = 1.0 ;
  int niter = 0 ;
  int validF = 1 ;

  arma::mat beta(p, 1, arma::fill::zeros) ;
  arma::mat beta_old = beta ;

  arma::mat VNT(r, r) ;
  arma::mat FE_inter_use(T, N, arma::fill::zeros) ;
  arma::mat covar_fit(T, N, arma::fill::zeros) ;
  arma::mat fit(T, N, arma::fill::zeros) ;
  arma::mat U ;

  arma::mat FE_ad(T, N, arma::fill::zeros) ;
  arma::mat alpha(N, 1, arma::fill::zeros) ;
  arma::mat xi(T, 1, arma::fill::zeros) ;
  double mu_Y = 0 ;
  arma::mat alpha_Y(N, 1, arma::fill::zeros) ;
  arma::mat xi_Y(T, 1, arma::fill::zeros) ;

  arma::mat YY_demean(T, N, arma::fill::zeros) ;

  arma::mat YY = Y ;

  List pf ;
  List Y_fe_ad ;
  List Y_ad ; 

  arma::mat F ;
  arma::mat L ;

  while (dif > tolerate && niter <= 500) {
    YY =  E_adj (Y, fit, I) ; // e-step: expectation
    
    // m1: estimate beta and add fe
    Y_ad = Y_demean(YY, force) ;
    YY_demean = as<arma::mat>(Y_ad["YY"]) ;
    beta = panel_beta(XX, xxinv, YY_demean, FE_inter_use) ;

    mu_Y = as<double>(Y_ad["mu_Y"]) ;
    if (force==1||force==3) {
      alpha_Y = as<arma::mat>(Y_ad["alpha_Y"]) ;
    }
    if (force==2||force==3) {
      xi_Y = as<arma::mat>(Y_ad["xi_Y"]) ;
    }
    Y_fe_ad = fe_add(alpha_X, xi_X, mu_X, alpha_Y, xi_Y, mu_Y, 
                          beta, T, N, p, force) ;
    covar_fit.zeros() ;
    for (int i = 0; i < p; i++) {
      covar_fit = covar_fit + XX.slice(i) * beta(i) ;
    }
    if (force == 0) {
      covar_fit = covar_fit + mu_Y ;
    }
    if (force == 1) {
      covar_fit = covar_fit + repmat(alpha_Y.t(), T, 1) ;
    }
    if (force == 2) {
      covar_fit = covar_fit + repmat(xi_Y, 1, N) ;
    }
    if (force == 3) {
      covar_fit = covar_fit + repmat(alpha_Y.t(), T, 1) + repmat(xi_Y, 1, N) - mu_Y ;
    }
    
    // m2: estimate interactive fe
    U = YY - covar_fit ;

    if (mc == 0) {
      pf = panel_factor(U, r)  ;
      F = as<arma::mat>(pf["factor"]) ;
      L = as<arma::mat>(pf["lambda"]) ;
      VNT = as<arma::mat>(pf["VNT"]) ;
      FE_inter_use = F * L.t() ; // interactive fe
    }
    else {
      FE_inter_use = panel_FE(U, lambda) ;
    }

    fit = covar_fit + FE_inter_use ; // overall fe 

    dif = arma::norm(beta - beta_old, "fro")/p ;
    beta_old = beta ;

    niter = niter + 1 ;
  }
  arma::mat e = YY - fit ;
  e = FE_adj(e, I) ;

  if (arma::accu(abs(FE_inter_use)) < 1e-10) {
    validF = 0 ;
  }

  List result;
  result["mu"] = as<double>(Y_fe_ad["mu"]) ;
  result["niter"] = niter ;
  result["e"] = e ;
  result["beta"] = beta ;
  result["fit"] = fit ;
  result["validF"] = validF ;

  if (force==1||force==3) {
    alpha = as<arma::mat>(Y_fe_ad["alpha"]) ;
    result["alpha"] = alpha ;
  }
  if (force==2||force==3) {
    xi = as<arma::mat>(Y_fe_ad["xi"]) ;
    result["xi"] = xi ;
  }

  if (mc == 0) {
    result["lambda"] = L ;
    result["factor"] = F ;
    result["VNT"] = VNT ;
  }
  return(result) ;
}

/* Main iteration for beta */
// [[Rcpp::export]]
List beta_iter (arma::cube X,
                arma::mat xxinv,
                arma::mat Y,
                int r,
                double tolerate,
                arma::mat beta0) {

  /* beta.new: computed beta under iteration with error precision=tolerate
     factor: estimated factor
     lambda: estimated loadings
     V: the eigenvalues matrix
     e: estimated residuals
     niter: number of interations to achieve convergence */
  int T = Y.n_rows ;
  int N = Y.n_cols ;
  int p = X.n_slices ;
  int b_r = beta0.n_rows ; 
  double beta_norm = 1.0 ;
  arma::mat beta(p, 1, arma::fill::zeros) ;
  if (b_r == p) {
      beta = beta0 ;
  } // beta should have the same dimension as X, if not it will be reset to 0
  arma::mat beta_old = beta ;
  arma::mat VNT(r, r, arma::fill::zeros) ;
  arma::mat FE(T, N, arma::fill::zeros) ;

  /* starting value */
  arma::mat U = Y ;
  for (int k = 0; k < p; k++) {
    U = U - X.slice(k) * beta(k) ;
  }
  List pf = panel_factor(U, r)  ;
  arma::mat F = as<arma::mat>(pf["factor"]) ;
  arma::mat L = as<arma::mat>(pf["lambda"]) ;
 
  /* Loop */
  int niter = 0 ;
  while ((beta_norm > tolerate) && (niter < 500)) {
    niter++ ; 
    FE = F * L.t() ;
    beta = panel_beta(X, xxinv, Y, FE) ;
    beta_norm = arma::norm(beta - beta_old, "fro") ; 
    beta_old = beta ;
    U = Y ;
    for (int k = 0; k < p; k++) {
      U = U - X.slice(k) * beta(k) ;
    }
    pf = panel_factor(U, r)  ;
    F = as<arma::mat>(pf["factor"]) ;
    L = as<arma::mat>(pf["lambda"]) ; 
  }
  VNT = as<arma::mat>(pf["VNT"]) ; 
  arma::mat e = U - F * L.t() ;

  /* Storage */
  List result ;
  result["niter"] = niter ;
  result["beta"] = beta ;
  result["e"] = e ; 
  result["lambda"] = L ;
  result["factor"] = F ;
  result["VNT"] = VNT ;
  return(result)  ;
}

/* Main iteration for beta: unbalanced without additive fixed effects,
   useless under the assumption of non-zero grandmean */
// [[Rcpp::export]]
List beta_iter_ub (arma::cube X,
                   arma::mat xxinv,
                   arma::mat Y,
                   arma::mat I,
                   int r,
                   double tolerate,
                   arma::mat beta0) { 
  int T = Y.n_rows ;
  int N = Y.n_cols ;
  int p = X.n_slices ;
  int b_r = beta0.n_rows ; 
  double beta_norm = 1.0 ;
  arma::mat beta(p, 1, arma::fill::zeros) ;
  if (b_r == p) {
      beta = beta0 ;
  }

  arma::mat beta_old = beta ;  
  arma::mat VNT(r, r, arma::fill::zeros) ;
  arma::mat FE(T, N, arma::fill::zeros) ;
  arma::mat FE_use(T, N, arma::fill::zeros) ; // intermediate value
  arma::mat U_use(T, N, arma::fill::zeros) ; // intermediate value

  /* starting value */
  arma::mat U = Y ;
  for (int k = 0; k < p; k++) {
    U = U - X.slice(k) * beta(k) ;
  }
  List pf = panel_factor(U, r)  ;
  arma::mat F = as<arma::mat>(pf["factor"]) ;
  arma::mat L = as<arma::mat>(pf["lambda"]) ;
 
  /* Loop */
  int niter = 0 ;
  while ((beta_norm > tolerate) && (niter < 500)) {
    niter++ ;
    FE = F * L.t() ;
    /* estimate beta */
    // set missing value = 0
    FE_use = FE_adj(FE = FE, I = I) ;
    beta = panel_beta(X, xxinv, Y, FE_use) ;
    beta_norm = arma::norm(beta - beta_old, "fro")/p ; 
    beta_old = beta ;
    U = Y ;
    for (int k = 0; k < p; k++) {
      U = U - X.slice(k) * beta(k) ;
    }
    /* estimate interactive fe */
    // Expectation for missing value
    U_use = E_adj(U, FE, I) ;
    pf = panel_factor(U_use, r)  ;
    F = as<arma::mat>(pf["factor"]) ;
    L = as<arma::mat>(pf["lambda"]) ; 
  }
  VNT = as<arma::mat>(pf["VNT"]) ; 
  FE = F * L.t() ;
  FE_use = FE_adj(FE = FE, I = I) ;
  arma::mat e = U - FE_use ;

  /* Storage */
  List result ;
  result["niter"] = niter ;
  result["beta"] = beta ;
  result["e"] = e ; 
  result["lambda"] = L ;
  result["factor"] = F ;
  result["VNT"] = VNT ;
  return(result)  ;
}

/* Interactive Fixed Effects */
// [[Rcpp::export]]
List inter_fe (arma::mat Y,
               arma::cube X,
               int r,
               int force,
               arma::mat beta0, 
               double tol = 1e-5
               ) { 
  /* Dimensions */
  int b_r = beta0.n_rows ; 
  int T = Y.n_rows ;
  int N = Y.n_cols ;
  int p = X.n_slices ;
  int obs = T * N ;
  int niter = 0 ;
  arma::mat factor ;
  arma::mat lambda ;
  arma::mat VNT ;
  arma::mat beta ; 
  arma::mat U ;
  double mu = 0 ;
  double mu_Y = 0 ;
  arma::mat mu_X(p, 1) ;
  arma::mat alpha(N, 1, arma::fill::zeros) ;
  arma::mat alpha_Y(N, 1) ;
  arma::mat alpha_X(N, p) ;
  arma::mat xi(T, 1, arma::fill::zeros) ;
  arma::mat xi_Y(T, 1) ;
  arma::mat xi_X(T, p) ;
  double sigma2 ;
  double IC ;
  //arma::mat FE(T, N, arma::fill::zeros) ;
  arma::mat invXX ;

  /* duplicate data */
  arma::mat YY = Y;
  arma::cube XX = X;
   
  /* grand mean */
  mu_Y  =  accu(YY)/obs ;
  YY  =  YY - mu_Y ;
  if (p > 0) {
    for (int i = 0; i < p; i++) {
      mu_X(i,0)  =  accu(XX.slice(i))/obs ;
      XX.slice(i) =  XX.slice(i)- mu_X(i,0) ;
    }
  }
  
  /* unit fixed effects */
  if (force ==1 || force ==3 ) {
    alpha_Y  =  mean(YY, 0).t() ; // colMeans, (N * 1) matrix
    YY  =  YY - repmat(alpha_Y.t(), T, 1) ; // (T * N) matrix 
    if (p > 0) {
      for (int i = 0; i < p; i++) {
        alpha_X.col(i)  = mean(XX.slice(i), 0).t(); // colMeans 
        XX.slice(i) = XX.slice(i) - repmat(alpha_X.col(i).t(), T, 1) ;
      }
    } 
  }
  
  /* time fixed effects  */
  if ( force == 2 || force == 3 ) {
    xi_Y  =  mean(YY, 1); //rowMeans, (N * 1) matrix
    YY  =  YY - repmat(xi_Y, 1, N);
    if (p > 0) {
      for (int i = 0; i < p; i++) {
        xi_X.col(i)  =  mean(XX.slice(i), 1) ; //rowMeans
        XX.slice(i) =  XX.slice(i) - repmat(xi_X.col(i), 1, N) ;
      }
    }  
  }

  /* check if XX has enough variation */
  int p1=p; 
  arma::mat X_invar(p, 1, arma::fill::zeros); // =1 if invar

  int j = 0;
  for(int i=0; i<p1; i++){
    if (arma::accu(abs(XX.slice(i))) < 1e-5) {
      XX.shed_slice(i);
      mu_X.shed_row(i);
      alpha_X.shed_col(i);
      xi_X.shed_col(i);
      X_invar(j,0) = 1;
      i--;
      p1--;
    }
    j++;
  }

  int validX = 1;
  if(p1==0){
    validX = 0;
  }
  else {
    invXX =  XXinv(XX) ;
  }
 
  /* Main Algorithm */ 
  if (p1 == 0) {
    if (r > 0) {
      List pf = panel_factor(YY, r)  ;
      factor = as<arma::mat>(pf["factor"]) ;
      lambda = as<arma::mat>(pf["lambda"]) ;
      VNT = as<arma::mat>(pf["VNT"]) ;
      U  =  YY - factor * lambda.t() ;
    } 
    else {
      U = YY ;
    } 
  } 
  else {
    /* starting value:  the OLS/LSDV estimator */
    if (accu(abs(beta0))< 1e-10 || r==0 || b_r != p1 ) {  //
      beta0 = panel_beta(XX, invXX, YY, arma::zeros<arma::mat>(T,N)); //
    }
    if (r==0) {
      U = YY;
      beta  =  beta0 ;
      for (int k = 0; k < p1; k++) {
        U =  U - XX.slice(k) * beta(k,0);
      }
    } 
    else if (r > 0) {  
      // arma::mat invXX =  XXinv(XX) ;  // compute (X'X)^{-1}, outside beta iteration      
      List out  =  beta_iter(XX, invXX, YY, r, tol, beta0) ;
      beta  = as<arma::mat>(out["beta"]) ;
      factor  =  as<arma::mat>(out["factor"]) ;
      lambda  =  as<arma::mat>(out["lambda"]) ;
      VNT  =  as<arma::mat>(out["VNT"]) ;
      U  =  as<arma::mat>(out["e"]);
      niter = as<int>(out["niter"]) ;
    }
  } 
    
  /* save fixed effects */
  if (p1 == 0) {
    
    mu =  mu_Y;
    
    if (force ==1 || force ==3) {
      alpha =  alpha_Y ;
    }
    if (force == 2 || force == 3) {
      xi =  xi_Y;
    }   
  } else { // with valid covariates
    
    mu  =  mu_Y - crossprod(mu_X, beta)(0,0) ;
    
    if (force ==1 || force == 3) {
      alpha  =  alpha_Y - alpha_X * beta ; 
    }
    if (force == 2 || force == 3) {
      xi  =  xi_Y - xi_X * beta  ;
    } 
  }

  /* sigma2 and IC */
  sigma2 = trace(U * U.t())/ (N * T - r * (N + T) + pow(double(r),2) - p1 ) ;
  
  IC = log(sigma2) + (r * ( N + T ) - pow(double(r),2) + p1) * log ( double(N * T) ) / ( N * T ) ;
    
  //-------------------------------#
  // Storage
  //-------------------------------# 

  List output ;
  
  output["mu"] = mu ;
  // output["p1"] = p1 ; 
 
  if(p>0) {
    // output["beta_valid"] = beta ;
    arma::mat beta_total(p,1);
    // arma::mat beta_tot(p,1);

    if(p>p1) {
      int j4= 0;
      for(int i=0; i<p; i++) {
        if(X_invar(i,0)==1) {
          beta_total(i,0) = arma::datum::nan;
          // beta_tot(i,0) = 0;
        }
        else {
          beta_total(i,0) = beta(j4,0);
          // beta_tot(i,0) = beta(j4,0);
          j4++;
        }
      }
    }
    else {
      beta_total = beta;
      // beta_tot = beta;
    }
    output["beta"] = beta_total;
    // output["beta_tot"] = beta_tot;
  }  

  if (r > 0) {
    output["factor"] = factor ;
    output["lambda"] = lambda ;
    output["VNT"] = VNT ;
    //FE = factor * lambda.t() ;
    //output["FE"] = FE ;
  }
  if ((p1 > 0) && (r > 0)) {
    output["niter"] = niter ;
  }
  if (force ==1 || force == 3) {
    output["alpha"] = alpha ;
  }
  if (force ==2 || force == 3) {
    output["xi"] = xi ;
  }
  output["residuals"] = U ;
  output["sigma2"] = sigma2 ;
  output["IC"] = IC ;
  output["validX"] = validX ;
  return(output);
}

/* Interactive Fixed Effects: ub */
// [[Rcpp::export]]
List inter_fe_ub (arma::mat Y,
                  arma::cube X,
                  arma::mat I,
                  int r, // r > 0, the outcome has a factor-type fixed effect; r = 0 else
                  int force,
                  double tol = 1e-5
                  ) {
  
  /* Dimensions */
  int T = Y.n_rows ;
  int N = Y.n_cols ;
  int p = X.n_slices ;
  double obs = accu(I) ;
  int niter = 0 ;
  arma::mat factor ;
  arma::mat lambda ;
  arma::mat VNT ;
  arma::mat beta ; 
  arma::mat U ;
  double mu_Y = 0 ;
  double mu = 0 ;
  arma::mat mu_X(p, 1, arma::fill::zeros) ;
  arma::mat alpha(N, 1, arma::fill::zeros) ;
  arma::mat alpha_Y(N, 1, arma::fill::zeros) ;
  arma::mat alpha_X(N, p, arma::fill::zeros) ;
  arma::mat xi(T, 1, arma::fill::zeros) ;
  arma::mat xi_Y(T, 1, arma::fill::zeros) ;
  arma::mat xi_X(T, p, arma::fill::zeros) ;
  arma::mat fit(T, N, arma::fill::zeros) ;
  double sigma2 = 0 ;
  double IC = 0 ;

  arma::mat invXX ;
  //arma::mat subX(T, N, arma::fill::zeros) ;

  /* duplicate data */
  arma::mat YY = Y;
  arma::cube XX = X;

  
  // mu_X 
  if (p > 0) {
    for (int i = 0; i < p; i++) {
      mu_X(i,0)  =  accu(XX.slice(i))/(N*T) ;
      if (force == 0) {
        XX.slice(i) =  XX.slice(i)- mu_X(i,0) ;
      }
    }
  }

  /* unit fixed effects */
  if (force == 1) {
    if (p > 0) {
      for (int i = 0; i < p; i++) {
        alpha_X.col(i)  = mean(XX.slice(i), 0).t(); // colMeans 
        XX.slice(i) = XX.slice(i) - repmat(alpha_X.col(i).t(), T, 1) ;
      }
    } 
  }
  
  /* time fixed effects  */
  if (force == 2) {
    if (p > 0) {
      for (int i = 0; i < p; i++) {
        xi_X.col(i)  =  mean(XX.slice(i), 1) ; //rowMeans
        XX.slice(i) =  XX.slice(i) - repmat(xi_X.col(i), 1, N) ;
      }
    }  
  }

  if (force == 3) {
    if (p > 0) {
      for (int i = 0; i < p; i++) {
        alpha_X.col(i)  = mean(XX.slice(i), 0).t(); // colMeans 
        xi_X.col(i)  =  mean(XX.slice(i), 1) ; //rowMeans
        XX.slice(i) = XX.slice(i) - repmat(alpha_X.col(i).t(), T, 1) 
          - repmat(xi_X.col(i), 1, N) + mu_X(i,0) ;
      }
    }
  }

  /* check if XX has enough variation */
  int p1 = p; 
  arma::mat X_invar(p, 1, arma::fill::zeros); // =1 if invar

  int j = 0;

  for(int i=0; i<p1; i++){
    if (arma::accu(abs(XX.slice(i))) < 1e-5) {
      XX.shed_slice(i);
      mu_X.shed_row(i);
      alpha_X.shed_col(i);
      xi_X.shed_col(i);
      X_invar(j,0) = 1;
      i--;
      p1--;
    }
    j++;
  }

  int validX = 1 ;
  if(p1==0){
    validX = 0 ;
    if (force == 0 && r == 0) { // no covariate and force == 0 and r == 0 
      mu_Y = accu(YY)/obs ;
      mu = mu_Y ;
      YY = FE_adj(YY - mu_Y, I) ;
    }
  }

  /* Main Algorithm */ 
  if (p1 == 0) {
    if (r > 0) {
      // add fe ; inter fe ; iteration
      List fe_ad_inter = fe_ad_inter_iter(YY, I, force, 0, r, 0, tol) ;
      mu = as<double>(fe_ad_inter["mu"]) ;
      U = as<arma::mat>(fe_ad_inter["e"]) ;
      fit = as<arma::mat>(fe_ad_inter["fit"]) ;

      factor = as<arma::mat>(fe_ad_inter["factor"]) ;
      lambda = as<arma::mat>(fe_ad_inter["lambda"]) ;
      VNT = as<arma::mat>(fe_ad_inter["VNT"]) ;

      if (force==1||force==3) {
        alpha = as<arma::mat>(fe_ad_inter["alpha"]) ;
      }
      if (force==2||force==3) {
        xi = as<arma::mat>(fe_ad_inter["xi"]) ;
      }
      niter = as<int>(fe_ad_inter["niter"]) ;
    } 
    else {
      if (force==0) {
        U = YY ;
        fit.fill(mu) ;
      } else {
        // add fe; iteration
        List fe_ad = fe_ad_iter(YY, I, force, tol) ;
        mu = as<double>(fe_ad["mu"]) ;
        U = as<arma::mat>(fe_ad["e"]) ;
        fit = as<arma::mat>(fe_ad["fit"]) ;
        if (force==1||force==3) {
          alpha = as<arma::mat>(fe_ad["alpha"]) ;
        }
        if (force==2||force==3) {
          xi = as<arma::mat>(fe_ad["xi"]) ;
        }
        niter = as<int>(fe_ad["niter"]) ;
      }
    } 
  } 
  else {
    /* starting value:  the OLS estimator */
    invXX = XXinv(XX) ; // compute (X'X)^{-1}, outside beta iteration 
    if (r==0) {
      // add fe, covar; iteration
      List fe_ad = fe_ad_covar_iter(XX, invXX, alpha_X, xi_X, mu_X,
                                    YY, I, force, tol) ;
      mu = as<double>(fe_ad["mu"]) ;
      beta = as<arma::mat>(fe_ad["beta"]) ;
      U = as<arma::mat>(fe_ad["e"]) ;
      fit = as<arma::mat>(fe_ad["fit"]) ;
      if (force==1||force==3) {
        alpha = as<arma::mat>(fe_ad["alpha"]) ;
      }
      if (force==2||force==3) {
        xi = as<arma::mat>(fe_ad["xi"]) ;
      }
      niter = as<int>(fe_ad["niter"]) ;
    } 
    else if (r > 0) {       
      // add, covar, interactive, iteration
      List fe_ad_inter_covar = fe_ad_inter_covar_iter(XX, invXX,
               alpha_X, xi_X, mu_X, YY, I, force, 0, r, 0, tol) ;
      mu = as<double>(fe_ad_inter_covar["mu"]) ;
      beta = as<arma::mat>(fe_ad_inter_covar["beta"]) ;
      U = as<arma::mat>(fe_ad_inter_covar["e"]) ;
      fit = as<arma::mat>(fe_ad_inter_covar["fit"]) ;

      factor = as<arma::mat>(fe_ad_inter_covar["factor"]) ;
      lambda = as<arma::mat>(fe_ad_inter_covar["lambda"]) ;
      VNT = as<arma::mat>(fe_ad_inter_covar["VNT"]) ;

      if (force==1||force==3) {
        alpha = as<arma::mat>(fe_ad_inter_covar["alpha"]) ;
      }
      if (force==2||force==3) {
        xi = as<arma::mat>(fe_ad_inter_covar["xi"]) ;
      }
      niter = as<int>(fe_ad_inter_covar["niter"]) ;
    }
  } 
    
  /* sigma2 and IC */
  sigma2 = trace(U * U.t())/ (obs - r * (N + T) + pow(double(r),2) - p1 ) ;

  IC = log(sigma2) + (r * ( N + T ) - pow(double(r),2) + p1)
   * log ( obs ) / ( obs ) ;
    
  //-------------------------------#
  // Storage
  //-------------------------------# 

  List output ;
  // output["p1"] = p1 ;
  // output["beta_valid"] = beta ;

  if(p>0) {
    // output["beta_valid"] = beta ;
    arma::mat beta_total(p,1);
    // arma::mat beta_tot(p,1);

    if(p>p1) {
      int j4= 0;
      for(int i=0; i<p; i++) {
        if(X_invar(i,0)==1) {
          beta_total(i,0) = arma::datum::nan;
          // beta_tot(i,0) = 0;
        }
        else {
          beta_total(i,0) = beta(j4,0);
          // beta_tot(i,0) = beta(j4,0);
          j4++;
        }
      }
    }
    else {
      beta_total = beta;
      // beta_tot = beta;
    }
    output["beta"] = beta_total;
    // output["beta_tot"] = beta_tot;
  }

  output["mu"] = mu ;   
  output["fit"] = fit ;  

  if ( !(force == 0 && r == 0 && p1 == 0) ) {
    output["niter"] = niter ;
  }
  if (force ==1 || force == 3) {
    output["alpha"] = alpha ;
  }
  if (force ==2 || force == 3) {
    output["xi"] = xi ;
  }
  if (r > 0) {
    output["factor"] = factor ;
    output["lambda"] = lambda ;
    output["VNT"] = VNT ;
    //FE = factor * lambda.t() ;
    //output["FE"] = FE ;
  }
  output["residuals"] = U ;
  output["sigma2"] = sigma2 ;
  output["IC"] = IC;
  output["validX"] = validX;
  return(output);
 
}


/* Interactive Fixed Effects: matrix completion */
// [[Rcpp::export]]
List inter_fe_mc (arma::mat Y,
                  arma::cube X,
                  arma::mat I,
                  int r, // r > 0, the outcome has a factor-type fixed effect; r = 0 else
                  double lambda,
                  int force,
                  double tol = 1e-5
                  ) {
  
  /* Dimensions */
  int T = Y.n_rows ;
  int N = Y.n_cols ;
  int p = X.n_slices ;
  double obs = accu(I) ;
  int niter = 0 ;
  int validF = 1 ;
  //arma::mat factor ;
  //arma::mat lambda ;
  //arma::mat FE_0(T, N, arma::fill::zeros) ;
  //arma::mat FE(T, N, arma::fill::zeros) ;
  //arma::mat VNT ;
  arma::mat beta ; 
  arma::mat U ;
  double mu_Y = 0 ;
  double mu = 0 ;
  arma::mat mu_X(p, 1, arma::fill::zeros) ;
  arma::mat alpha(N, 1, arma::fill::zeros) ;
  arma::mat alpha_Y(N, 1, arma::fill::zeros) ;
  arma::mat alpha_X(N, p, arma::fill::zeros) ;
  arma::mat xi(T, 1, arma::fill::zeros) ;
  arma::mat xi_Y(T, 1, arma::fill::zeros) ;
  arma::mat xi_X(T, p, arma::fill::zeros) ;
  arma::mat fit(T, N, arma::fill::zeros) ;
  double sigma2 = 0;
  //double IC ;

  arma::mat invXX ;
  //arma::mat subX(T, N, arma::fill::zeros) ;

  /* duplicate data */
  arma::mat YY = Y;
  arma::cube XX = X;

  // mu_X 
  if (p > 0) {
    for (int i = 0; i < p; i++) {
      mu_X(i,0)  =  accu(XX.slice(i))/(N*T) ;
      if (force == 0) {
        XX.slice(i) =  XX.slice(i)- mu_X(i,0) ;
      }
    }
  }

  /* unit fixed effects */
  if (force == 1) {
    if (p > 0) {
      for (int i = 0; i < p; i++) {
        alpha_X.col(i)  = mean(XX.slice(i), 0).t(); // colMeans 
        XX.slice(i) = XX.slice(i) - repmat(alpha_X.col(i).t(), T, 1) ;
      }
    } 
  }
  
  /* time fixed effects  */
  if (force == 2) {
    if (p > 0) {
      for (int i = 0; i < p; i++) {
        xi_X.col(i)  =  mean(XX.slice(i), 1) ; //rowMeans
        XX.slice(i) =  XX.slice(i) - repmat(xi_X.col(i), 1, N) ;
      }
    }  
  }

  if (force == 3) {
    if (p > 0) {
      for (int i = 0; i < p; i++) {
        alpha_X.col(i)  = mean(XX.slice(i), 0).t(); // colMeans 
        xi_X.col(i)  =  mean(XX.slice(i), 1) ; //rowMeans
        XX.slice(i) = XX.slice(i) - repmat(alpha_X.col(i).t(), T, 1) 
          - repmat(xi_X.col(i), 1, N) + mu_X(i,0) ;
      }
    }
  }

  /* check if XX has enough variation */
  int p1 = p; 
  arma::mat X_invar(p, 1, arma::fill::zeros); // =1 if invar

  int j = 0;

  for(int i=0; i<p1; i++){
    if (arma::accu(abs(XX.slice(i))) < 1e-5) {
      XX.shed_slice(i);
      mu_X.shed_row(i);
      alpha_X.shed_col(i);
      xi_X.shed_col(i);
      X_invar(j,0) = 1;
      i--;
      p1--;
    }
    j++;
  }

  int validX = 1 ;
  if(p1==0){
    validX = 0 ;
    if (force == 0 && r == 0) { // no covariate and force == 0 and r == 0 
      mu_Y = accu(YY)/obs ;
      mu = mu_Y ;
      YY = FE_adj(YY - mu_Y, I) ;
    }
  }

  /* Main Algorithm */ 
  if (p1 == 0) {
    if (r > 0) {
      // add fe ; inter fe ; iteration
      List fe_ad_inter = fe_ad_inter_iter(YY, I, force, 1, 0, lambda, tol) ;
      mu = as<double>(fe_ad_inter["mu"]) ;
      U = as<arma::mat>(fe_ad_inter["e"]) ;
      fit = as<arma::mat>(fe_ad_inter["fit"]) ;
      if (force==1||force==3) {
        alpha = as<arma::mat>(fe_ad_inter["alpha"]) ;
      }
      if (force==2||force==3) {
        xi = as<arma::mat>(fe_ad_inter["xi"]) ;
      }
      niter = as<int>(fe_ad_inter["niter"]) ;
      validF = as<int>(fe_ad_inter["validF"]) ;
    } 
    else {
      if (force==0) {
        U = YY ;
        fit.fill(mu) ;
        validF = 0 ;
      } else {
        // add fe; iteration
        List fe_ad = fe_ad_iter(YY, I, force, tol) ;
        mu = as<double>(fe_ad["mu"]) ;
        U = as<arma::mat>(fe_ad["e"]) ;
        fit = as<arma::mat>(fe_ad["fit"]) ;
        if (force==1||force==3) {
          alpha = as<arma::mat>(fe_ad["alpha"]) ;
        }
        if (force==2||force==3) {
          xi = as<arma::mat>(fe_ad["xi"]) ;
        }
        niter = as<int>(fe_ad["niter"]) ;
        validF = 0 ;
      }
    } 
  } 
  else {
    /* starting value:  the OLS estimator */
    invXX = XXinv(XX) ; // compute (X'X)^{-1}, outside beta iteration 
    if (r==0) {
      // add fe, covar; iteration
      List fe_ad = fe_ad_covar_iter(XX, invXX, alpha_X, xi_X, mu_X,
                                    YY, I, force, tol) ;
      mu = as<double>(fe_ad["mu"]) ;
      beta = as<arma::mat>(fe_ad["beta"]) ;
      U = as<arma::mat>(fe_ad["e"]) ;
      fit = as<arma::mat>(fe_ad["fit"]) ;
      if (force==1||force==3) {
        alpha = as<arma::mat>(fe_ad["alpha"]) ;
      }
      if (force==2||force==3) {
        xi = as<arma::mat>(fe_ad["xi"]) ;
      }
      niter = as<int>(fe_ad["niter"]) ;
      validF = 0 ;
    } 
    else if (r > 0) {       
      // add, covar, interactive, iteration
      List fe_ad_inter_covar = fe_ad_inter_covar_iter(XX, invXX,
               alpha_X, xi_X, mu_X, YY, I, force, 1, 0, lambda, tol) ;
      mu = as<double>(fe_ad_inter_covar["mu"]) ;
      beta = as<arma::mat>(fe_ad_inter_covar["beta"]) ;
      U = as<arma::mat>(fe_ad_inter_covar["e"]) ;
      fit = as<arma::mat>(fe_ad_inter_covar["fit"]) ;
      if (force==1||force==3) {
        alpha = as<arma::mat>(fe_ad_inter_covar["alpha"]) ;
      }
      if (force==2||force==3) {
        xi = as<arma::mat>(fe_ad_inter_covar["xi"]) ;
      }
      niter = as<int>(fe_ad_inter_covar["niter"]) ;
      validF = as<int>(fe_ad_inter_covar["validF"]) ;
    }
  } 
    
  /* sigma2 and IC */
  sigma2 = trace(U * U.t())/ (obs - r * (N + T) + pow(double(r),2) - p1 ) ;
    
  //-------------------------------#
  // Storage
  //-------------------------------# 

  List output ;
  // output["p1"] = p1 ;
  // output["beta_valid"] = beta ;

  if(p>0) {
    // output["beta_valid"] = beta ;
    arma::mat beta_total(p,1);
    // arma::mat beta_tot(p,1);

    if(p>p1) {
      int j4= 0;
      for(int i=0; i<p; i++) {
        if(X_invar(i,0)==1) {
          beta_total(i,0) = arma::datum::nan;
          // beta_tot(i,0) = 0;
        }
        else {
          beta_total(i,0) = beta(j4,0);
          // beta_tot(i,0) = beta(j4,0);
          j4++;
        }
      }
    }
    else {
      beta_total = beta;
      // beta_tot = beta;
    }
    output["beta"] = beta_total;
    // output["beta_tot"] = beta_tot;
  }

  output["mu"] = mu ;   
  output["fit"] = fit ; 
  output["validF"] = validF ; 

  if ( !(force == 0 && r == 0 && p1 == 0) ) {
    output["niter"] = niter ;
  }
  if (force ==1 || force == 3) {
    output["alpha"] = alpha ;
  }
  if (force ==2 || force == 3) {
    output["xi"] = xi ;
  }
  output["residuals"] = U ;
  output["sigma2"] = sigma2 ;
  output["validX"] = validX;
  return(output);
 
}

