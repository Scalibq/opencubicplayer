% Open Cubic Player Documentation LaTeX class
% most of this has been shamelessly stolen from the Python documentation
%

\NeedsTeXFormat{LaTeX2e}[1995/12/01]
\ProvidesClass{ocpmanual}[1999/05/08 Document class (OpenCP manual)]

\LoadClass[a4paper,10pt,twoside,openright]{book}

\newcommand{\py@doHorizontalRule}{\mghrulefill{\RW}}
\RequirePackage{ocpstyle}

% dieser hack gibts uns eine schoeeeene titlepage

\renewcommand{\maketitle}{%
  \begin{titlepage}%
    \thispagestyle{realempty}
    \let\footnotesize\small
    \let\footnoterule\relax
    \begin{center}%
      \mbox{}
      \vfill
      {\rm\Huge\@title\par}%
      \py@doHorizontalRule\par%
      {\rm\Large\longtitle\par}
      \vfill
      {\large\begin{tabular}[t]{c}\@author\end{tabular}\par}
      \vfill\vfill
      {\@date\par\vfill}%
    \end{center}%\par
  \end{titlepage}%
  \setcounter{footnote}{0}%
  \let\thanks\relax\let\maketitle\relax
  \gdef\@thanks{}\gdef\@author{}\gdef\@title{}
}

% This is needed to get the width of the section # area wide enough in the
% library reference.  Doing it here keeps it the same for all the manuals.
%
\renewcommand*\l@section{\@dottedtocline{1}{1.5em}{2.6em}}
\renewcommand*\l@subsection{\@dottedtocline{2}{4.1em}{3.5em}}
\setcounter{tocdepth}{1}


% Fix the theindex environment to add an entry to the Table of
% Contents; this is much nicer than just having to jump to the end of
% the book and flip around, especially with multiple indexes.
%
\let\cp@OldTheindex=\theindex
\renewcommand{\theindex}{
  \cleardoublepage
  \cp@OldTheindex
  \addcontentsline{toc}{chapter}{\indexname}
}
