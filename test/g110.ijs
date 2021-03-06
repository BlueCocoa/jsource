1:@:(dbr bind Debug)@:(9!:19)2^_44[(echo^:ECHOFILENAME) './g110.ijs'
NB. *y ------------------------------------------------------------------

qct  =: 2^_44
sgn  =: (%|) * >:!.0&qct@|
test =: sgn -: *

test x=:1=?2 5$2
test x=:_10+?13$20
test x=:o._10+?5 10$20
test x=:^0j1*_10+?5 10$20

s=:_1 0 1

test s*  qct
test s*+:qct
test s*-:qct

test 0j1*s*  qct
test 0j1*s*+:qct
test 0j1*s*-:qct

s -: *!.0 s*qct

t -: * t=:1=i.0 0
t -: * t=:0j1*i.0 5

dr  =: 5!:2
rk  =: 1 : ('+"u b. 0')

0 0 0 -: * rk

1 _1 -: * }. 1j1 _ __

0 0j1 0j_1 -: * 0 j. 0 _ __
1 _1 = * _ __ j. 0
(<'NaN error') = * etx&.> j./~ _ __

'domain error' -: * etx 'abc'
'domain error' -: * etx u:'abc'
'domain error' -: * etx 10&u:'abc'
'domain error' -: * etx <12


NB. x*y -----------------------------------------------------------------

4 = type 1234* 5678
4 = type 1234*_5678
8 = type ". > IF64 {'123456* 123456';'1e15* 1e15'
8 = type ". > IF64 {'123456*_123456';'1e15*_1e15'

(10^>:i.20)           -: */\20$10
((20$_1 1)*10^>:i.20) -: */\20$_10

'domain error' -: 'abc' * etx 4
'domain error' -: 'abc' *~etx 4
'domain error' -: 4     * etx <'abc'
'domain error' -: 4     *~etx <'abc'
'domain error' -: (u:'abc') * etx 4
'domain error' -: (u:'abc') *~etx 4
'domain error' -: 4     * etx <u:'abc'
'domain error' -: 4     *~etx <u:'abc'
'domain error' -: (10&u:'abc') * etx 4
'domain error' -: (10&u:'abc') *~etx 4
'domain error' -: 4     * etx <10&u:'abc'
'domain error' -: 4     *~etx <10&u:'abc'
'domain error' -: (s:@<"0 'abc') * etx 4
'domain error' -: (s:@<"0 'abc') *~etx 4
'domain error' -: 4     * etx s:@<"0&.> <'abc'
'domain error' -: 4     * etx <"0@s: <'abc'
'domain error' -: 4     *~etx s:@<"0&.> <'abc'
'domain error' -: 4     *~etx <"0@s: <'abc'

'length error' -: 3 4   * etx 5 6 7
'length error' -: 3 4   *~etx 5 6 7
'length error' -: 3 4   * etx i.5 6
'length error' -: 3 4   *~etx i.5 6


NB. x*y integer overflow handling ---------------------------------------

test  =: * 0&=@:- *&((o.0)&+)
testa =: (*/   0&=@:- */  @((o.0)&+))@,
testb =: (*/\  0&=@:- */\ @((o.0)&+))@,
testc =: (*/\. 0&=@:- */\.@((o.0)&+))@,
x     =: <._1+2^31
y     =: <.-2^31

4 = type&> _1 0 1 *&.> x
4 = type&> _1 0 1 *&.>~x
4 = type&>    0 1 *&.> -x
4 = type&>    0 1 *&.>~-x
4 = type&> _1 0 1 */@,&.> x
4 = type&> _1 0 1 */@,&.>~x
4 = type&>    0 1 */@,&.> -x
4 = type&>    0 1 */@,&.>~-x
4 = type&> _1 0 1 */\@,&.> x
4 = type&> _1 0 1 */\@,&.>~x
4 = type&>    0 1 */\@,&.> -x
4 = type&>    0 1 */\@,&.>~-x
4 = type&> _1 0 1 */\.@,&.> x
4 = type&> _1 0 1 */\.@,&.>~x
4 = type&>    0 1 */\.@,&.> -x
4 = type&>    0 1 */\.@,&.>~-x

f =: 4&=@type@* *. test

2     f 1e9
2     f~1e9
61034 f 32768
61034 f~32768
65536 f 32767
65536 f~32767
65535 f 32768
65535 f~32768

46340  f  46341
46340  f _46341
_46340 f  46341
_46340 f _46341

,(2e6 2e6 _2e6 _2e6) test &> 3e7 _3e7 3e7 _3e7
,(2e6 2e6 _2e6 _2e6) testa&> 3e7 _3e7 3e7 _3e7
,(2e6 2e6 _2e6 _2e6) testb&> 3e7 _3e7 3e7 _3e7
,(2e6 2e6 _2e6 _2e6) testc&> 3e7 _3e7 3e7 _3e7

,_1 0 1 test &>/ 1234567 _1234567
,_1 0 1 test &>/~1234567 _1234567
,_1 0 1 testa&>/ 1234567 _1234567
,_1 0 1 testa&>/~1234567 _1234567
,_1 0 1 testb&>/ 1234567 _1234567
,_1 0 1 testb&>/~1234567 _1234567
,_1 0 1 testc&>/ 1234567 _1234567
,_1 0 1 testc&>/~1234567 _1234567

,(<.2^30) test &> <. 2^i.10
,(<.2^30) test &>~<. 2^i.10
,(<.2^30) test &> <.-2^i.10
,(<.2^30) test &>~<.-2^i.10
,(<.2^30) testa&> <. 2^i.10
,(<.2^30) testa&>~<. 2^i.10
,(<.2^30) testa&> <.-2^i.10
,(<.2^30) testa&>~<.-2^i.10
,(<.2^30) testb&> <. 2^i.10
,(<.2^30) testb&>~<. 2^i.10
,(<.2^30) testb&> <.-2^i.10
,(<.2^30) testb&>~<.-2^i.10
,(<.2^30) testc&> <. 2^i.10
,(<.2^30) testc&>~<. 2^i.10
,(<.2^30) testc&> <.-2^i.10
,(<.2^30) testc&>~<.-2^i.10

,_2 _1 0 1 2 test &>/ x,y
,_2 _1 0 1 2 test &>/ x,y
,_2 _1 0 1 2 testa&>/~x,y
,_2 _1 0 1 2 testa&>/ x,y
,_2 _1 0 1 2 testb&>/~x,y
,_2 _1 0 1 2 testb&>/~x,y
,_2 _1 0 1 2 testc&>/~x,y
,_2 _1 0 1 2 testc&>/~x,y

12e6  test  8e5
12e6  test _8e5
_12e6 test  8e5
_12e6 test _8e5

12e6 testa 8e5
12e6 testb 8e5
12e6 testc 8e5

exp =: * * | ^ i.@>:@<.@(^.&2147483647)@|
f   =: *./@,@(test"0/)&exp " 0

x    f/ x =: 2 3 5 6 7 10 11 12 13
x    f/-x
(-x) f/ x
(-x) f/-x

eq =: -:&(3!:1)

( 1 0*_ ) eq  2   0*_
( 1 0*_ ) eq  2.5 0*_
(_1 0*_ ) eq _2   0*_
(_1 0*_ ) eq _2.5 0*_
( 1 0*__) eq  2   0*__
( 1 0*__) eq  2.5 0*__
(_1 0*__) eq _2   0*__
(_1 0*__) eq _2.5 0*__


NB. x*y on complex arguments --------------------------------------------

times=: (-/@:* j./ +/@:(* |.))&+.

x=: j./?2 20 100$50
y=: j./?2 20 100$50
x           (* -: times"0) y
x           (* -: times"0) y*?($y)$2
(x*?($x)$2) (* -: times"0) y
(x*?($x)$2) (* -: times"0) y*?($y)$2


4!:55 ;:'dr eq exp f qct rk s sgn t test testa testb testc times x y'


