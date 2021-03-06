[+:   -*- nroff -*-
AutoGen5 template man=%s.1

## ---------------------------------------------------------------------
## agman1.tpl -- Template for command line man pages
##
##  AutoOpts copyright 1992-2002 Bruce Korb
##
## Author:          Jim Van Zandt <jrv@vanzandt.mv.com>
## Maintainer:      Bruce Korb <bkorb@gnu.org>
## Created:         Mon Jun 28 15:35:12 1999
## Time-stamp:      "2002-02-01 16:02:34 bkorb"
##              by: bkorb
## ---------------------------------------------------------------------
## $Id: man1.tpl,v 1.1 2002/09/07 15:17:53 raarts Exp $
## ---------------------------------------------------------------------

(setenv "SHELL" "/bin/sh")

:+]
.TH [+: % prog_name (string-upcase! "%s") :+] 1 [+:
  `date +%Y-%m-%d` :+] "" "Programmer's Manual"
[+:

;; The following "dne" argument is a string of 5 characters:
;; '.' '\\' '"' and two spaces.  It _is_ hard to read.
;;
(dne ".\\\"  ")

:+]
.\"
.SH NAME
[+: % prog_name (string-downcase! "%s") :+] \- [+:prog_title:+]
.SH SYNOPSIS[+:

      # * * * * * * * * * * * * * * * * * * * * * * * * *
      #
      #  Display the command line prototype,
      #  based only on the argument processing type.
      #
      :+]
.B [+: (string-downcase! (get "prog_name")) :+][+:

  IF (exist? "flag.value")  :+][+:
    IF (exist? "long_opts") :+][+:

      # * * * * *
      #
      :+]
.\" Mixture of short (flag) options and long options
.RB [ -\fIflag\fP " [\fIvalue\fP]]... [" --\fIopt-name\fP " [[=| ]\fIvalue\fP]]..."[+:

    ELSE no long options:+][+:

      # * * * * *
      #
      :+]
.\" Short (flag) options only
.RB [ -\fIflag\fP " [\fIvalue\fP]]..."[+:
    ENDIF :+][+:

  ELIF (exist? "long_opts") :+][+:

      # * * * * *
      #
      :+]
.\" Long options only
.RB [ --\fIopt-name\fP [ = "| ] \fIvalue\fP]]..."[+:

  ELIF  (not (exist? "argument")) :+][+:

      # * * * * *
      #
      :+]
.\" All arguments are named options.
.RI [ opt-name "[\fB=\fP" value ]]...
.PP
All arguments are named options.[+:

  ELSE :+][+:
    (error "Named option programs cannot have arguments") :+][+:
  ENDIF :+][+:

  IF (exist? "argument") :+]
.br
.in +8
[+:argument:+][+:
  ELIF (or (exist? "long_opts") (exist? "flag.value")) :+]
.PP
All arguments must be options.[+:
  ENDIF :+][+:

      # * * * * * * * * * * * * * * * * * * * * * * * * *
      #
      #  Describe the command.  Use 'prog_man_desrip' if it exists,
      #  otherwise use the 'detail' help option.  If not that,
      #  then the thing is undocumented.
      #
      :+][+:
IF (exist? "explain") :+]
.PP
[+:explain:+][+:
ENDIF :+]
.SH "DESCRIPTION"
[+:

IF (exist? "prog_man_descrip")   :+][+:
  FOR prog_man_descrip "\n.PP\n" :+][+:
    prog_man_descrip             :+][+:
  ENDFOR                         :+][+:
ELIF (exist? "detail")           :+][+:
  FOR detail  "\n.PP\n"          :+][+:
    detail                       :+][+:
  ENDFOR                         :+][+:
ELSE
  :+]Its description is not documented.[+:
ENDIF :+]
.SH OPTIONS[+:

# * * * * * * * * * * * * * * * * * * * * * * * * *
#
   Describe each option

:+][+:
(define opt-arg  "")
(define opt-name "")            :+][+:

IF (exist? "preserve-case")     :+][+:
  (define optname-from "_^")
  (define optname-to   "--")    :+][+:
ELSE                            :+][+:
  (define optname-from "A-Z_^")
  (define optname-to   "a-z--") :+][+:
ENDIF                           :+][+:

FOR flag

:+][+:
  ;;  Skip the documentation options!
  ;;
  (set! opt-name (string-tr! (get "name") optname-from optname-to))
  (if (not (exist? "arg_type"))
      (set! opt-arg "")
      (set! opt-arg (string-append "\\fI"
            (if (exist? "arg_name") (get "arg_name")
                (string-downcase! (get "arg_type")))
            "\\fP" ))
  )
  :+][+:
  IF (not (exist? "documentation")) :+]
.TP[+:
    IF (exist? "value") :+][+:
      IF (exist? "long_opts") :+][+:

          # * * * * * * * * * * * * * * * * * * * *
          *
          *  The option has a flag value (character) AND
          *  the program uses long options
          *
          :+]
.BR -[+:value:+][+:
          IF (not (exist? "arg_type")) :+] ", " --[+:
          ELSE  :+] " [+:(. opt-arg):+], " --[+:
          ENDIF :+][+: (. opt-name)    :+][+:
          IF (exist? "arg_type")       :+][+:
              ? arg_optional " [ =" ' "=" '
              :+][+:  (. opt-arg)      :+][+:
              arg_optional " ]"        :+][+:
          ENDIF:+][+:


        ELSE   :+][+:

          # * * * * * * * * * * * * * * * * * * * *
          *
          *  The option has a flag value (character) BUT
          *  the program does _NOT_ use long options
          *
          :+]
.BR -[+:value:+][+:
          IF (exist? "arg_type") :+][+:
            arg_optional "["     :+] "[+:(. opt-arg):+][+:
            arg_optional '"]"'   :+][+:
          ENDIF:+][+:
        ENDIF  :+][+:


      ELSE  value does not exist -- named option only  :+][+:

        IF (not (exist? "long_opts")) :+][+:

          # * * * * * * * * * * * * * * * * * * * *
          *
          *  The option does not have a flag value (character).
          *  The program does _NOT_ use long options either.
          *  Special magic:  All arguments are named options.
          *
          :+]
.BR [+: (. opt-name) :+][+:
           IF (exist? "arg_type") :+] [+:
              ? arg_optional " [ =" ' "=" '
              :+][+:(. opt-arg):+][+:
              arg_optional "]" :+][+:
           ENDIF:+][+:


        ELSE   :+][+:

          # * * * * * * * * * * * * * * * * * * * *
          *
          *  The option does not have a flag value (character).
          *  The program, instead, only accepts long options.
          *
          :+]
.BR --[+: (. opt-name) :+][+:
          IF (exist? "arg_type") :+] "[+:
            arg_optional "["     :+]=[+:(. opt-arg):+][+:
            arg_optional "]"     :+]"[+:
          ENDIF:+][+:
        ENDIF  :+][+:
      ENDIF :+]
[+:descrip:+].[+:
      IF (exist? "min") :+]
This option is required to appear.[+:ENDIF:+][+:
      IF (exist? "max") :+]
This option may appear [+:
          IF % max (= "%s" "NOLIMIT")
          :+]an unlimited number of times[+:ELSE
          :+]up to [+:max:+] times[+:ENDIF:+].[+:ENDIF:+][+:
      IF (exist? "enabled") :+]
This option is enabled by default.[+:ENDIF:+][+:
      IF (exist? "no_preset") :+]
This option may not be preset with environment variables
or in initialization (rc) files.[+:ENDIF:+][+:
      IF (exist? "equivalence") :+]
This option is a member of the [+:equivalence:+] class of options.[+:ENDIF:+][+:
      IF (exist? "flags_must") :+]
This opton must appear in combination with the following options:
[+: FOR flags_must ", " :+][+:flags_must:+][+:ENDFOR:+].[+:ENDIF:+][+:
      IF (exist? "flags_cant") :+]
This option must not appear in combination with any of the following options:
[+: FOR flags_cant ", " :+][+:flags_cant:+][+:ENDFOR:+].[+:ENDIF:+]
.sp
[+: # SED
   convert @code, @var and @samp into \fB...\fP phrases
   convert @file into \fI...\fP phrases
   convert @var into \fB...\fP phrases
   Remove the '@' prefix from curly braces
   Indent example regions
   Delete example command
   Replace "end example" command with ".br"
   Replace "@*" command with ".br"

   NB:  backslashes are interpreted three times: by AutoGen, sed and nroff.
        Thus, for nroff to see one backslash, use four (4)
        and for nroff to output a backslash, use EIGHT (8)!!!!

   ):+][+: % doc `sed \
 -e 's;@code{\\([^}]*\\)};\\\\fB\\1\\\\fP;g' \
 -e  's;@var{\\([^}]*\\)};\\\\fB\\1\\\\fP;g' \
 -e 's;@samp{\\([^}]*\\)};\\\\fB\\1\\\\fP;g' \
 -e 's;@file{\\([^}]*\\)};\\\\fI\\1\\\\fP;g' \
 -e 's/@\\([{}]\\)/\\1/g' \
 -e 's,^\\$\\*$,.br,' \
 -e '/@ *example/,/@ *end *example/s/^/    /' \
 -e '/^ *@ *example/d' \
 -e 's/^ *@ *end *example/.br/' \
 -e '/^ *@ *enumerate/d' \
 -e 's/^ *@ *end *enumerate/.br/' \
 -e '/^ *@ *table/d' \
 -e 's/^ *@ *end *table/.br/' \
 -e 's/^@item/.sp 1/' \
 -e 's/^@\\*/.br/' <<'_EOF_'\n%s\n_EOF_` :+][+:

  ENDIF documentation _exist ! :+][+:

ENDFOR flag


:+]
.TP
.BR [+:IF (exist? "flag.value") :+]\-? , " --[+:
      ELIF (exist? "long_opt") :+]--[+:
      ENDIF:+]help
Display usage information and exit.
.TP
.BR [+:IF (exist? "flag.value") :+]-! , " --[+:
      ELIF (exist? "long_opt") :+]--[+:
      ENDIF:+]more-help
Extended usage information passed thru pager.[+:


IF (exist? "homerc") :+]
.TP
.BR [+: IF (exist? "flag.value") :+]-> " \fIrcfile\fP, --" [+:
      ELIF (exist? "long_opt") :+]--[+:
      ENDIF:+]save-opts "[=\fIrcfile\fP]
Save the option state to \fIrcfile\fP.
.TP
.BR [+: IF (exist? "flag.value") :+]-< " \fIrcfile\fP, --" [+:
      ELIF (exist? "long_opt") :+]--[+:
      ENDIF:+]load-opts "=\fIrcfile\fP"
Load options from \fIrcfile\fP.
.TP
.BR  --no-load-opts
Disable loading options from an rc file.[+:
ENDIF (exist? "homerc") :+][+:


IF (exist? "version") :+]
.TP
.BR [+:  IF (exist? "flag.value") :+]\-v " \fI[v|c|n]\fP, " --[+:
      ELIF (exist? "long_opt") :+]--[+:ENDIF:+]version "\fI[=v|c|n]\fP"
Output version of program and exit.  The default mode is `v', a simple
version.  The `c' mode will print copyright information and `n' will
print the full copyright notice.[+:
ENDIF


:+][+:
IF (exist? "explain") :+]
.PP
[+:explain:+][+:
ENDIF:+][+:
IF (exist? "man_doc") :+]
[+:man_doc:+][+:
ENDIF:+][+:

IF (or (exist? "copyright.author") (exist? "copyright.owner")) :+]
.SH AUTHOR
[+: ?% copyright.author '%s' (get "copyright.owner")
  :+][+:
  IF (exist? "copyright.eaddr") :+] <[+: copyright.eaddr :+]>[+:
  ELIF (exist? "eaddr")         :+] <[+: eaddr :+]>[+:
  ENDIF    :+][+:

  CASE copyright.type :+][+:
   =  gpl  :+]
.PP
Released under the GNU General Public License.[+:
   = lgpl  :+]
.PP
Released under the GNU General Public License with Library Extensions.[+:
   =  bsd  :+]
.PP
Released under the Free BSD License.[+:
   *       :+][+:
     IF (exist? "copyright.text")
           :+]
.PP
.nf
.na
[+: copyright.text :+]
.fi
.ad[+:
     ELIF (exist? "copyright.date") :+]
.PP
Released under an unspecified copyright license.[+:
     ENDIF :+][+:
  ESAC     :+][+:
ENDIF      :+]
[+: #

man.tpl ends here  :+]
