 *********************************************************
    Mother Board Monitor Program for X Window System

        XMBmon ver.2.03

    for FreeBSD (and for Linux and NetBSD/OpenBSD).
 *********************************************************


<< 0. どんなソフトか >>

　最近はマザーボードには CPU の温度や CPU Fan の回転数などをモニター
する機能が付いています。マイクロソフト Windows 上ではこのモニター機能を
使ってリアルタイムに CPUの温度などをグラフ化するプログラムがありますが、
X 上でそのような機能を持ったプログラムがないようなので作ってみました。
ただし、機能は最低限のものしかありません。コマンドライン版 mbmon では、
温度・電圧・ファンの回転数をリアルタイムに報告し、X Window System版
xmbmon では 3つの温度とコア電圧を曲線グラフにして表示します。


<< 1. 新ヴァージョンでの変更点 >>
------------------------------------------------------------------
(ver. 2.02 ---> ver. 2.03)

・Genesys Logic の GL512SM, GL520SM センサーをサポート。

・インテルの ICH5 の SMBus アクセスをサポート。

・LM85、Analog Devices ADM1024/1025/1027/ADT7463、
 および SMSC EMC6D10X センサーをサポート。

・Analog Devices ADM1020/1021/1023 温度センサーをサポート。

・versin 2.00 からあった IT78xxF のファンディバイザーのバグを
 フィクス。

・その他、細かいバグの fix、コードのクリーンアップ。
------------------------------------------------------------------
(ver. 2.01 ---> ver. 2.02)

・NetBSD/OpenBSD でも使えるようになった(Stephan Eisvogel さんの
 パッチによる)。「./configure; make」も動く。

・AMD8111 と NVidia nForce2 の SMBus アクセスをサポート
 (Alex van Kaam 氏の情報による)。

・LM90 温度センサーをサポート。

・Winbond の W83L784R, W83L785R, W83L785TS-S センサーをサポート。

・2つのセンサーチップを使っている場合に対応
 (詳しくは 4. 使い方 参照)。
------------------------------------------------------------------
(ver. 2.00 ---> ver. 2.01)

・mbmon を daemon として起動するオプションを追加(Jean-Marc Zucconi
 さんによるパッチを統合)。これにより、telnet によりネットワーク越し
 に mbmon の output を得ることができる(4. 使い方 参照)。

・ASUS ASB100 (Bach) sensor chip の取扱いを改善(Alex van Kaam さん
  からの情報による)。少なくとも、ASUS A7V333/A7V8X のマザーボードに
  においては Athlon XP の内蔵センサーからの温度を得ることができる
  ようになった(2番目の temp. として)。

・ASUS Mozart-2 sensor chip をサポートした(Alex van Kaam さんから
  の情報による)。

・xmbmon が NFS の ~/.Xauthority ファイルを読めないことがある点を
  修正(中村和志＠神戸さんからのパッチ)
------------------------------------------------------------------
(ver. 2.00)

・いくつかの新しいハードウエアモニタチップ、National Semiconductor
  LM75、LM80 (長村さんによる)、ASUS ASB100 Bach、をサポート。

・いくつかの新しい SMBus インターフェイス chipset、AMD7xx、
  NVidia nForce、 ALi MAGiK1/AladdinPro、をサポート。

・ハードウエアモニタチップの自動判別機能を強化するとともに、モニタ
  チップ名を指定することもできるようにした。

・特殊仕様の Tyan製マザーボード TigerMP/MPX に対応。

・xmbmon を用いずに独自のグラフィックス表示をしたい場合(例えば、MRTG や
  rrdtool を用いる)のための mbmon の出力機能の更なる強化(長村さんによる
  rrdtool用の出力とそれを用いた perl script, mbmon-rrd.pl を添付)。

・複数の SMBus を使用している人のために、FreeBSD の SMBus driver
  を用いる時の device file名の番号 X (/dev/smbX) を指定できるように
  した(これは FreeBSD のみで、Linux には関係ない)。

・プログラムの大幅な見直しをして、新しいモニタチップや SMBus インター
  フェイスに対応し易くした(00READMEtech.txt 参照、ただし英語のみ)。

・負電圧の出力値を一部見直したので、これまでのヴァージョンの出力とは
  異なる場合がある。

・その他、細かいバグの fix。


<< 2. サポートしているハードウエアモニタチップ >>

　もし、自分のマザーボードに積まれているハードウエア監視機能が以下の
チップおよびその互換チップを使っている場合には動作する可能性があります。

   National Semiconductor社          LM78/LM79, LM75, LM90, LM80, LM85
   WinBond社                         W83781D, W83782D, W83783S,
                                     W83627HF, W83697HF 内蔵,
                                     W83L784R, W83L785R, W83L785TS-S
   ASUSTek社                         AS99127F, ASB100(Bach),
                                     ASM58 etc. (Mozart-2)
   VIA Technology社                  VT82C686A/B 内蔵
   Integrated Technology Express社   IT8705F, IT8712F 内蔵
   Genesys Logic社                   GL518SM, GL520SM
   Analog Devices社                  ADM1027, ADT7463, ADM1020/1021/1023
   Standard Microsystem Corporation  EMC6D100/101


<< 3. サポートしている SMBus インタフェイスチップ >>

　ハードウエアモニタチップへのインタフェイスとして、ISA-IO port を
用いる方法、VIA VT82C686A/B チップ専用の方法 (いづれも後述)、それ以外
の一般的方法としては SMBus (System Management Bus) があるが、これを
実現しているチップ毎にアクセスの方法が異なります。現在のところ、
以下のようなチップ(多くの場合チップセットのサウスブリッジ)に対応。

   Intel: PIIX4 (440BX), ICH, ICH0, ICH2, ICH3, ICH4,
                         ICH5 (810, 815, ...)
   VIA: VT82C596/B, VT82C686A/B (KT133/A), VT8233A/C (KT266/A/KT333),
                    VT8235(KT400)
   AMD: AMD756(AMD750), AMD766(AMD760), AMD768(AMD760MP), AMD8111
   NVidia: nForce, nForce2
   Acer Lab. Inc.(ALi): M1535D+(MAGiK1/AladdinPro)

ただし、必ずしもすべてのチップセットを使ったマザーボードでチェック
されているわけではなく、動くことを保証はできません。

　なお、実機で動作チェックしたマザーボードは以下の通り(ただし、過去の
動作報告、ベータテスターの方のマザーボードも含む):

   ABIT: VP6(ApolloPro133A(VIA686B)), KT7A(KT133A),
         KG7(AMD761+VIA686A), NF7(nForce2/ISA+W83627HF)
   ASUS: P2B,P2B-F,P2B-B(440BX+W83781D), P3B-F(440BX+AS99127F),
         K7VM(KT133+W83782D), A7V,A7V133(KT133/A+AS99127F),
         A7A266(ALi M1535D++AS99127F), A7V266(KT266/A+AS99127F),
         A7N266(NVidia nForce+AS99127F), A7M266-D(AMD768+AS99127F),
         A7V333(KT333+ASB100), A7V8X(KT400+ASB100),
         A7N8X(nForce2+ASB100+W83L785TS-S),
         CUSL(Intel815E+AS99127F), TUSI-M(SiS630/ISA+IT8705F),
         P4B533-VM(Intel845(ICH4)+ASM58), P4PE(Intel845PE(ICH4)+ASM100)
   EpoX: BX6SE(440BX/ISA+W83782D), 8KHA+(KT266A/ISA+W83697HF)
   ECS:  K7S5A(Sis735/ISA+IT8705F), D6VAA(AppoloPro133A(VIA686B))
   Soltek: SL75DRV2(KT266+IT8705F)
   Freeway: FW-6280BXDR/155(440BX+W83781D/W83782S)
   Aopen: MX3S(i815E(ICH2)+W83627H), MX36LE(PLE133(VIA686B))
   Shuttle: FV24(PLE133(VIA686B))
   MSI:  K7N2 Delta-L(nForce2/ISA+W83627HF)
   Tyan: TigerMP/MPX(S2460/S2462)(AMD766+W83782D+W83627HF)[Note],
         Trinity 100AT(VIA586A(viapm)+GL518SM)
   Gigabyte: GA-7VAXP(KT400/ISA+IT8705F+LM90),
             GA-SINXP-1394(SiS655/ISA+IT8705F)
   Leadtek: K7NCR18D(nForce2+W83783S + W83L785TS-S)
   Albatron: KX400+Pro(KT333/ISA+W83697HF + W83L785TS-S)
   Aopen: AX4GER(Intel845GE(ICH4)/ISA+W83627HF)
   Intel: S845WD1(Tentel845E(ICH4)+LM85)

[注] Tyan TigerMP/MPX については、6.オプションのところの最後の[注]
  を読んでください。


<< 4. 使い方 >>

　パッケージを解凍してできたディレクトリで ./configure & make すれば

      mbmon      motherboard monitor for tty-terminal
      xmbmon     motherboard monitor for X

の二つのバイナリができます。

((注意)) このプログラムは SMBus または ISA-IO port に問答無用で
   アクセスし、場合によってはシステムをクラッシュさせる可能性が
   ある「危険」なソフトウエアです。このことを十分認識して下さい。
   特に、IO port 0x295, 0x296 へのアクセスは NE2000 互換ボードと
   重なる場合がありますので、これらの port への書き込みが許されるか
   チェックしてください。私はこのプログラムによって起きた如何なる
   問題にも責任は取れません。

　プログラムのコンパイルが成功したら、まず、あなたのマザーボードで
ちゃんと動くかどうかをチェックしてみましょう。まず、root になって

   # ./mbmon -d   (又は ./mbmon -d -A   以下を参照)

と mbmon プログラムをデバッグモードで動かしてみて下さい。これで、

   No Hardware Monitor found!!
   InitMBInfo: Undefined error: 0

というエラーメッセージが表示されずに、

   Using SMBus access method[VIA8233/A(KT266/KT333)]!!
   * Asus Chip, ASB100 found.

のように検知したハードウエアモニタチップの情報を表示すれば、この
プログラムが動く可能性があります。続けて -d オプションなしで起動して、

   Temp.= 31.0, 37.0, 37.0; Rot.= 3970, 2576, 2700
   Vcore = 1.74, 1.74; Volt. = 3.38, 5.08, 12.40, -11.35, -4.90

のような表示が 5秒おきに繰り返されるならば大丈夫です。もし、温度が
一つだけ表示され、あなたのマザーボードで電圧とファンスピードのハード
ウエアモニタ機能があることが間違いなければ、-A オプションを付け加えて
みてください(説明は次の段落にあります)。ちゃんと動くようなら、とりあえず
^C (CTRL-C)でプログラムを止めて 2つのプログラムをインストールして
下さい(mbmon は ^C 以外に止める方法はありません)。mbmon, xmbmon の
両方とも IO port に直接アクセスするので setuid root してパスの通った
ところに置いておく必要があります。Makefile の make install を参考に
してください。もし、mbmon を /usr/local/bin、xmbmon を /usr/X11R6/bin
に置くのでよければ、

   # make install

でこれらの作業をやってくれます。

　あるマザーボードでは 2つのセンサーチップが使われています。一つは主たる
ハードウエアモニタとして使われ、もう一つの補助的センサーは CPU温度を測る
ためのものです。 verion 2.02 からはそのような場合もサポートされました。
あなたのマザーボードで何個のセンサーチップが使われているかを見るためには、
mbmon を -A オプション付で次のように起動します:

   # ./mbmon -d -A

もし、結果が次のようになったすると、

   Summary of Detection:
    * SMB monitor(s)[VT8233/A/8235(KT266/KT333/KT400)]:
     ** Winbond Chip W83L785TS-S found at slave address: 0x5C.
    * ISA monitor(s):
     ** Winbond Chip W83697HF found.

主たるモニターチップとしては W83697HF が、補助的に CPU温度を測るセンサー
としては (CPU のオンダイのダイオードに接続されている)W83L785TS-Sが
使われていることを意味します。この場合、通常の使用法では、

   # ./mbmon

   Temp.= 76.0,  0.0,  0.0; Rot.=    0,    0,    0
   Vcore = 0.00, 0.00; Volt. = 0.00, 0.00,  0.00,   0.00,  0.00

   # ./mbmon -I

   Temp.= 41.0, 64.5,  0.0; Rot.= 4440,    0,    0
   Vcore = 1.66, 0.00; Volt. = 3.26, 4.76, 12.65, -12.20, -5.15

となりますが、-A オプション付だと両者の出力が以下のように統一され、

   # ./mbmon -A

   Temp.= 41.0, 64.5, 76.0; Rot.= 4440,    0,    0
   Vcore = 1.66, 0.00; Volt. = 3.26, 4.76, 12.65, -12.20, -5.15

現れた 3番目の温度の表示が、W83L785TS-S によって測られた CPU 温度(76.0度)
を示します。この例では、W83697HF は 2つの温度しか測れないので、3番目の
温度が自然に補助的センサー W83L785TS-S によって測られた温度値に置き換え
られました。しかし、もし主たるセンサーが 3つの温度を測ることができる
場合は、どの温度値を補助的センサーのものに置き換えるかを選ぶ必要があり
ます。このために、-e [0|1|2] というオプションがあり、それぞれ、1番目、
2番目、 3番目を置き換えることを意味します(デフォルトは 3番目)。今のところ、
補助的センサーとして CPU温度値を置き換えられるチップは以下の通りです:

     W83L785TS-S
	 LM90
	 LM75


　上にも書いたようにこのプログラムは IO port へのアクセスを行ないます
ので、例え setuid root してもこのアクセスが禁止されていれば動きません。
もし、

   InitMBInfo: Operation not permitted
   This program needs "setuid root"!!

となったとすると、FreeBSD のシステムの security level が厳しいために
IO port にアクセスできないと思われます(/etc/rc.conf 中の
kern_securelevel_enable, kern_securelevel 参照)。security level を
下げるか、すなおにあきらめてください。

　version 2.01 から、mbmon を デーモンモードで起動することができるように
なり、ネットワーク越しに mbmon の output を調べることができます。

   # mbmon -P port_no

調べたいホスト側でこうしておくと、別のマシンから telnet することにより、

   $ telnet hostname port_no
   Trying xxx.xxx.xxx.xxx...
   Connected to hostname.
   Escape character is '^]'.

   Temp.= 29.0, 25.0, 38.5; Rot.= 3750, 3688, 2410
   Vcore = 1.70, 1.70; Volt. = 3.31, 4.97, 12.40, -11.35, -4.87
   Connection closed by foreign host.

というように出力を得ることができます。

　なお、負の電圧(Volt. = の 4番目[-12V]と 5番目[-5V]) は多くのマザー
ボードで不正確な値を示すことがわかっています。これについては、メーカー
がハードウエアモニタチップのデータシートに従っていないことが原因なので、
対応のしようがありません。

　また、dual CPU (SMP) 仕様のマザーボードでは SMBus と ISA IO port
(詳しくは後述 5. より詳しい解説 を参照)の両方にモニタチップが繋がって
いる場合があります。./mbmon の結果がおかしい時には -I オプションを付けて
試してみてください。 

　Linux の場合は Makefile 中の DEFS 変数に -DLINUX を付けてコンパイル
する必要があります。もし、configure スクリプトが失敗してこれが付いて
いない場合は手動で付けてください。ただし、私の回りには実機で Linux の
動く環境はありません。 FreeBSD 上の Linux emulation 環境ではコンパイル
および動作のチェックは行なっていますの大丈夫だとは思いますが、動作しない
可能性もあります。


<< 5. より詳しい解説 >>

　このプログラムの最初のヴァージョンはマザーボードに積まれている
ハードウエアモニターチップとして Winbond の W83781D chip をメインに
考えられていました。このチップでは、温度が 3つ(Temp0, Temp1, Temp2)、
電圧が 7つ(V0 - V6)、および FAN の回転数が 3つ(Fan0, Fan1, Fan2) が
測定できるようになっています。 mbmon ではすべての情報をテキストで
xmbmon では Temp0,1,2 および V0(Vcore)の情報をグラフで表示します。
単位は温度については摂氏(C) [version 1.07 から華氏(F)の表示も可能]、
電圧についてはヴォルト(V)、 FAN の回転数については 回/分 (rpm) です。
現在では他のいくつかのチップもサポートされており、別のチップではこれら
の情報のうちいくつかはサポートされていません。例えば、VIA のVT82C686A/B
の場合は温度は 3つ、電圧は 5つ、ファンの回転数は 2つのみがモニターされ
ます。mbmon ではモニターされない項目は 0.0 または 0 と表示されます。
なお、mbmon は入力値としてモニター情報を表示する間隔(単位:秒、
デフォルトは 5秒)を受け付けます。

　xmbmon の場合 Temp0,Temp1,Temp2 をそれぞれ "MB","CPU","chip" という
項目で表示しますが、それらは X の resource として変更可能です
(xmbmon.resources 参照)。マザーボードによっては、これら 3つの温度が
実際に何をモニターした温度かは違っており、適切な項目名になっていない
と思いますので、適宜変更してください。また、2つしか温度センサーがない、
または、繋がっていない場合にはこれを表示させないようにできます。
そのためには Temp0,Temp1,Temp2 のいづれか 1つの項目を "NOTEMP"(または
ブランク '')に設定してください(詳しくは 6. のオプションの項参照)。

　自分のマザーボードでどのようなハードウエアモニターチップが使われて
いるかわからない場合は、先にも書いたように

   # mbmon -d         または、   # mbmon -D
   # xmbmon -debug               # xmbmon -DEBUG

というデバッグオプション付で起動することによりハードウエアモニター
チップの詳しいデバッグ情報を得ることができます(-D/-DEBUG がより詳しい
情報を表示する)。これによってこのプログラムが使えるかどうかがかなりの
程度判断できるでしょう。このプログラムでサポートされているハードウエア
モニタチップの内で、LM78/79 はマザーボードの温度 Temp0 のみであり、
場合によっては LM75 という温度のみがモニターできるチップが併用されて
いることがあります。 W83783S の場合は Temp3 がありません。また、マザー
ボードによってはモニターチップがサポートしていてもその機能を生かして
いないものもあるようです。なお、通常 Temp0 がマザーボードの温度で、
これはほとんど室温に近く変化に乏しいようです。Temp1,2 は温度センサー
チップを繋いで始めて有効になる場合が多いのですが、最近のマザーボード
(例えば ASUS P3B-F) では PentiumII/III の CPU 内部にある温度センサーに
Temp1 が繋がれていることもあるようです。

　一般に最近のマザーボードではハードウエアモニターチップにアクセス
する方法として、SMBus (System Management Bus)と 古い ISA の IO port
による方法がありますが、新しいチップ(例えば ASUS の マザーボードで多く
使われている AS99127F など)は SMBus のみでしか使えないものもあります。
ver.1.06 からは SMBus アクセスを行なう自前のファンクションを用意する
ことによって、OS の device driver のサポートのない場合にも SMBus への
アクセスができるようになっています(3. サポートしている SMBus インタ
フェイスチップ を参照)。現在の FreeBSD のヴァージョン(4.6R)では、
device driver として、intpm (440BX のサウスチップ PIIX4)、
amdpm(AMD756[注1])、alpm(ALi M15x3[注2])、viapm(VIA586/596/686/8233)
の 4つがサポートされており、xmbmonパッケージの SMBus アクセスを
使わずにこの device driver を使うためには、オプションとして Makefile
中の DEFS 変数に -DSMBUS_IOCTL を付け加えてみてください。./configure
で /dev/smb0 があることが確認されていれば、device driver の ioctl
アクセスファンクションにより SMBus にアクセスします。この時、一般に
SMBus は一般的なバスであり、ハードウエアモニターチップのみが使って
いるとは限りません。例えば、テレビチューナーカードで有名な Brooktree
のチップのための device driver bktr も /dev/smbX を使用します。従って、
もし、SMBus を使う device が複数ある場合には、dmesg の出力を見ることに
よって、どの device file がハードウエアモニターチップに対応するかを
知っておかなければなりません。もし、間違った device file を用いると
システムをハングアップさせたり、再起動させたりしてしまいます(私自身が
bktr0 との併用で経験済み)。そこで、この時の device file の番号、
/dev/smbX の X[0-9] を指定するオプションが ver.2.00 から新設されました。
これが -s/-smbdev です。

   # mbmon -s X[0-9]
   # xmbmon -smbdev X[0-9]

デフォルトでは X=0 (/dev/smb0)になっています。

　VIA の VT82C686A/B の内蔵ハードウエアモニタ機能の場合には Winbond
のチップとは違った方法で ISA-IO port にアクセスすることによって温度等
の情報を得ることができます。従って、これを合わせると全部で 3つの方法で
ハードウエアモニタチップとアクセスすることができます。この VIA チップに
特有の方法を method V、 より一般的なハードウエアモニタチップへのアクセス
方法である SMBus による方法を method S、そして LM78/79 や Winbond の
チップ、また、ITE の IT8705F などの ISA-IO port によるアクセス方法を
method I、として強制的にそれぞれの方法でハードウエアモニタから情報を
得るためのオプションが

   # mbmon -[V|S|I|A]
   # xmbmon -method [V|S|I|A]

です。マザーボードによってどの方法がサポートされているかを調べるには、
デバッグオプションに更にこのオプションを付けて起動してみるといいでしょう。
もし、これらの method オプションを付けない時には method V、S、I、の順序
でチェックして自動的に有効なアクセス方法を選択して起動するようにしてい
ます。なお、mbmon と xmbmon にはこの他にも多くのオプションがありますが、
これらはヘルプオプション -h (or -help)付きで起動することによって表示され
ます。また、xmbmon の場合、それらは X の resource として .Xdefaults
ファイルに記述して制御することもできます。これらは 6. オプションと
xmbmon の X resource について のところで詳しく説明されます。

　また、ver.2.00 からは、自分のマザーボードがどのハードウエアモニター
チップを使っているかを知っている場合に、他のチップと誤認識を防ぐため
にモニターチップをプローブさせるオプションを付けました。もし、自動
認識に失敗するようであれば試してみてください。現在のところ

   # mbmon -p [winbond|wl784|via686|it87|lm80|lm90|lm75]
   # xmbmon -probe [winbond|wl784|via686|it87|lm80|lm90|lm75]

の 7種類のチップ名を指定できます。ただし、それぞれは

  winbond:   LM78/LM79,W83781D, W83782D, W83783S,W83627HF,W83697HF,
             AS99127F, ASB100
  wl784:     W83L784R, W83L785R, W83L785TS-S
  via686:    VT82C686A/B
  it87:      IT8705F, IT8712F
  lm80:      LM80
  lm90:      LM90
  lm75:      LM75

に対応します。

[注1] amdpm はごく最近 NVidia nForce chipset の SMBus にも対応しました
   (ただし、まだ commit されてないようです)。また、ごく簡単なパッチで
   AMD766/768 にも対応させることができます([FreeBSD-users-jp 68570])。
   詳しい情報をご要望の方は私の方まで直接連絡をください。

[注2] alpm の対応チップセット ALi M15x3 は一世代古いチップで、最近の
   MAGiK1 や AlladinPro の M1535D+ に対応させるためには簡単なパッチが
   必要です。詳しい情報をご要望の方は私の方まで直接連絡をください。


<< 6. オプションと xmbmon の X resource について >>

  mbmon は以下のようなオプションを持っています。

   # mbmon -h
MotherBoard Monitor, ver. 2.02 by YRS.
Usage: mbmon [options...] <seconds for sleep> (default 5 sec)
 options:
  -V|S|I: access method (using "VIA686 HWM directly"|"SMBus"|"ISA I/O port")
  -d/D: debug mode (any other options except (V|S|I) will be ignored)
  -A: for probing all methods, all chips, and setting an extra sensor.
 [-s [0-9]: for using /dev/smb[0-9]\n" ]<== compiled with -DSMBUS_IOCTL
  -e [0-2]: set extra temperature sensor to temp.[0|1|2] (need -A).
  -p chip: chip=winbond|wl784|via686|it87|lm80|lm90|lm75 for probing chips
  -Y: for Tyan Tiger MP/MPX motherboard
  -h: print help message(this) and exit
  -f: temperature in Fahrenheit
  -c count: repeat <count> times and exit
  -P port: run in daemon mode, using given port for clients
  -T|F [1-7]: print Temperature|Fanspeed according to following styles
        style1: data1\n
        style2: data2\n
        style3: data3\n
        style4: data1\ndata2\n
        style5: data1\ndata3\n
        style6: data2\ndata3\n
        style7: data1\ndata2\ndata3\n
  -r: print TAG and Value format
  -u: print system uptime
  -t: print present time
  -n|N: print hostname(long|short style)
  -i: print integers in the summary(with -T option)

　ここで、-V|S|I は上で述べたハードウエアモニタチップへのアクセス法
を指定するもの、-f は温度を華氏で表示、-d/D はデバッグモードで起動
するもの、-s が /dev/smbX を指定するもの(SMBUS_IOCTL オプション付で
コンパイルした時のみ)、-p は使用しているハードウエアモニタチップを指定
するもの、-h はヘルプの表示です。-c 以下は mbmon の出力を利用するため
のオプションで、説明からわかると思いますが、-c count は count 回出力した
後終了する。-P は port番号を与えることにより、mbmon をデーモンモードで
起動します(4.の使い方参照)。 -T|Fは温度、または、ファンスピードを
下の style の形で出力するためのもの、-u, -t, -n|N は直接ハードウエア
モニタ機能とは関係ありませんが、マシンの他の情報を出力、-i は出力を
浮動小数点ではなく整数でするためのものなどです。

　version 2.02 では、CPU温度を測る余分なセンサーチップを使えるようにする
ために -A と -e [0|1|2] という新しいオプションが付け加えられました。
-A により余分なチップが存在するかどうかを調べ、もし可能ならそれを使用可能
にします。また、その出力をもとのセンサーの 1番目、2番目、3番目の温度値と
置き換えるために -e [0|1|2] オプションを用います。ただし、デフォルトでは
3番目の温度値と置換えを行ないます。詳しくは 4. 使い方 参照。

　なお、この強力な出力機能によって、MRTG や rrdtool といった外部
プログラムから容易に出力を利用可能です。特に、rrdtool のための perl
script が長村さんによって contribute されています。これを適当なところ
にインストールすれば、

   # mbmon-rrd.pl [rrdfile]

で、rrdtool 用のデータが生成できます。もちろん、これを使用するためには
rrdtool の package/port をインストールしておく必要があります。

　xmbmon は X toolkit を用いて作成されており、標準的な X resource
(geometry, font など)が通常通り使えます。この他に xmbmon に特有の
resource は以下の通りです(ファイル xmbmon.resources)。

XMBmon*count:   下の sec の間に温度等を調べる回数 (default:4)
XMBmon*sec:     グラフの 1点を書く間隔の秒数 (default:20)
XMBmon*wsec:    グラフの全幅に対応する秒数 (default:1800)
XMBmon*tmin:    温度の下限の目盛、度C単位 (default:10.0)
XMBmon*tmax:    温度の上限の目盛、度C単位 (default:50.0)
XMBmon*vmin:    コア電圧の下限の目盛、V単位 (default:1.80)
XMBmon*vmax:    コア電圧の上限の目盛、V単位 (default:2.20)
XMBmon*tick:    温度・コア電圧の目盛のティックの数 (default:3)
XMBmon*cltmb:   Temp0 の線の色 (default:blue)
XMBmon*cltcpu:  Temp1 の線の色 (default:red)
XMBmon*cltcs:   Temp2 の線の色 (default:cyan)
XMBmon*clvc:    Vcore の線の色 (default:green)
XMBmon*cmtmb:   Temp0 用の項目名、表示しない時は NOTEMP (default:\ MB)
XMBmon*cmtcpu:  Temp1 用の項目名、表示しない時は NOTEMP (default:CPU)
XMBmon*cmtcs:   Temp2 用の項目名、表示しない時は NOTEMP (default:chip)
XMBmon*cmvc:    Vcore 用の項目名 (default:Vc\ )
XMBmon*method:  使うアクセス方法 (default:\ )
XMBmon*extratemp: 余分なセンサーの読みを置き換える温度 (default: 2)
XMBmon*smbdev:  /dev/smbX device file の番号 (default:0)
XMBmon*fahrn:   温度を華氏で表示、True または False (default: False)
XMBmon*probe:   強制プローブさせるチップ名 (default:\ )
XMBmon*TyanTigerMP:  Tyan Tiger MP/MPX motherboard用 (default:False)
XMBmon*label:   表示するラベル (default:なし)
XMBmon*labelcolor: ラベルの色 (default: black)

　デフォルトでは 20(sec)/4(count)=5 秒毎にタイマー割り込みを発生させて
温度等の情報を読みとり、20秒間の 4回の読みとり値の平均値をグラフに
プロットすることになります。xmbmon のウインドウの横幅に対応する時間が
1800(wsec)秒、すなわち30分間で、もしこれを過ぎると以前のグラフの値は
消去されグラフは新しい値を更新していきます(ちょうど xload と同じように)。
以上の resource は xmbmon のとれるオプションと同じで、 xmbmon を起動する
時に指定することもできます。-help オプションを付けて起動するとこの説明が
次のように出ます:

   # xmbmon -help
X MotherBoard Monitor, ver. 2.02 by YRS.
  options:
    : -g      (100x140) <geometry(Toolkit option)>
    : -count        (4) <counts in an interval>
    : -sec         (20) <seconds of an interval>
    : -wsec      (1800) <total seconds shown>
    : -tmin      (10.0) <min. temperature>
    : -tmax      (50.0) <max. temperature>
    : -vmin      (1.80) <min. voltage>
    : -vmax      (2.20) <max. voltage>
    : -tick         (3) <ticks in ordinate>
    : -cltmb     (blue) <Temp1 color>
    : -cltcpu     (red) <Temp2 color>
    : -cltcs     (cyan) <Temp3 color>
    : -clvc     (green) <Vcore color>
    : -cmtmb      ( MB) <comment of Temp1> [Not shown  ]
    : -cmtcpu     (CPU) <comment of Temp2> [if "NOTEMP"]
    : -cmtcs     (chip) <comment of Temp3> [set.       ]
    : -cmvc       (Vc ) <comment of Vcore>
    : -fahrn    (False) <temp. in Fahrenheit (True|False)>
    : -label        ( ) for showing label [No label if null-string.]
                         and -labelfont, -labelcolor
    : -method       ( ) <access method (V|S|I)>
   [: -smbdev [0-9] (0) for using /dev/smb[0-9]<== compiled with -DSMBUS_IOCTL]
    : -extratemp    (2) set extra-temp. to temp[0|1|2] (need -A)
    : -probe chip   ( ) chip=winbond|wl784|via686|it87|lm80|lm90|lm75
                         for probing monitor chips
    : -TyanTigerMP      for Tyan Tiger MP motherboard
    : -debug/DEBUG      for debug information


[注] version 2.00 から、特殊仕様の Tyan製マザーボード Tiger MP/MPX
  (S2460/S2462) にも対応しました。ただし、このマザーボードの場合は 2つ
  センサーの内一つを初期設定する必要があります。このためにオプションを
  付けて起動してください(mbmon の場合は -Y、xmbmon の場合は -TyanTigerMP)。
  そうすると SMBus でアクセスすると第一のセンサー、ISA IO でアクセスすると
  第二のセンサーから値を読み出すことができるようになります。この時、2つ
  のセンサーの値は次のようになっているそうです。

   第一センサー(SMBus, W83782D)

   temp0      VRM2 温度
   temp1      CPU1 温度
   temp2      CPU2 温度
   Vcore0     CPU1 Vcore
   Vcore1     CPU2 Vcore
   Volt 0     AGP 電圧
   Volt 1     システム 5V
   Volt 2     DDR 電圧
   Volt 3     -----
   Volt 4     スタンドバイ 3.3V

   第二センサー(ISA IO, W83627HF)

   temp0      VRM1 温度
   temp1      AGP  温度
   temp2      DDR  温度
   Vcore0     CPU1 Vcore
   Vcore1     CPU2 Vcore
   Volt 0     システム 3.3V
   Volt 1     システム 5V
   Volt 2     システム 12V
   Volt 3     システム -12V
   Volt 4     -----


<< 7. バグ >>

　verion 1.06 までにあった、他のプログラムが /dev/io を open したまま
でいると xmbmon が暴走する、というバグは 1.06pl1 (patch level 1)で fix
されました。今のところこの他に致命的なバグはないと思います。

　ただし、これまで色々と動作報告をいただいた中で、マザーボードによっては
ハードウエアモニタチップのデータシート通りに実装されていない場合がある
ようです。その時は値が読みとれないか、読みとれてもそれが BIOS で示されて
いる値とはかなり異なるものになってしまいます。これに対しては、そのマザー
ボードベンダーの情報開示がない限り対応できません。


<< 8. 他のハードウエアモニタチップに対応させたい人へ >>

　version 2.00 では実はかなり大がかりなコードの変更を行ないました。
これは、異なる処理をするハードウエアモニタチップ毎にプログラムを分離
しモジュール化することによって、新しいチップに対応することを容易にする
ためです(sens_XXXX.c がそれぞれのモジュール)。また、SMBus へのアクセス方法
も通常チップセットのサウスチップに内蔵されている、Power Management
Controller 毎に異なっており、これに対してもプログラムを分離しモジュール化
しました(smbus_XXXX.c がそれぞれのモジュール)。といっても、今のところ
私自身がこれ以上のモニタチップや Power Management Controller に対応する
ことは考えていませんので、もし、現在のヴァージョンで対応していないハード
ウエアをお持ちで、ガッツのある方は是非 hack して対応させてください。
そして、その結果を公開していただくことを期待いたします(簡単な説明が
00READMEtech.txt にあります、ただし英語のみ)。

　そのような hack のためには、まず PCI bus に繋がっている device を
知ることから始めなければなりません。そのためには FreeBSD に含まれて
いる pciconf コマンドを使います。また、このパッケージには特にモニタ
チップのために PCI Configuration を調べるテストプログラムが testpci.c、
SMBus をチェックするプログラムが testsmb.c、また、 Winbond-like な
ハードウエアモニタ機能をチェックするプログラムが testhwm.c として
含まれています。hack の第一歩として使ってみてください。ただし、これら
は mbmon/xmbmon 自身より更に危険なプログラムであることは言うまでもあり
ませんので、OS のクラッシュなどの不慮の事態に備えることをお忘れなく
(私も一度ディスクコントローラを間違ってオープンして書き込みを行ない、
ファイルシステムを崩してしまいました)。

　なお、どのマザーボードがどのようなハードウエアモニタチップを使って
いるかに関する情報は例えば、マイクロソフト Windows 上の MBM
(Mother Board Monitor) というプログラムの開発者 Alex van Kaam さん
のホームページ、

   http://mbm.livewiredev.com/

が参考になるでしょう。または、日本では LM78mon というプログラムの
開発者である :p araffin.(米谷)さんのホームページ、

   http://homepage1.nifty.com/paraffin/

の support 掲示版での議論も参考になると思います。また、Winbond の
チップの詳しい情報は以下 Winbond のホームページに PDF ファイルとして
あります。

   http://www.winbond.com.tw/

または、Linux での hardware monitor (lm_sensor) のホームページにも
これらの多くのチップの PDF ファイルが集めてあります。

   http://www.lm-sensors.nu/

　しかしながら、一般にハードウエアモニタチップのアクセス方法の情報
は必ずしも公開されているとは限らず、その場合は残念ながら対応は
難しいかもしれません。


<< 9. 謝辞 >>

・最初の公開 version 1.04 では ISA IO port へのアクセスによって
  Winbond chip からの情報を得るものでした。この時、このアクセス方法
  については、上記の MBM の作者の Alex van Kaam さんが当時ホームページ
  で公開されていたものを用いました(残念ながら今この情報は見当たり
  ません)。この情報がなければこのプログラムの開発は行なわれなかった
  でしょう。大変感謝しています(現在詳しい情報は上記の Winbond のホーム
  ページ上のデータシート PDFファイルにあります)。

・次の公開 version 1.05 の作成に当たっては、SMBus 対応および configure
  への対応について FreeBSD の intpm driver を書かれた渡辺尊紀＠神戸大さん
  に全面的に御協力いただきました。version 1.05 では、この intpm に基づく
  SMBus への ioctl によるアクセス方法とは違うやり方を導入しましたが、
  device driver が導入された環境ではこの方法は安全な方法であり、また、
  このアクセス方法の分離の雛型を示していただいたことは、以降の開発に大変
  役にたちました。非常に感謝しています。また、この version 1.05 から
  Linux への部分的対応が行なわれましたが、それには 岡村耕二＠九大さん
  に協力していただきました。また、ASUS のチップ AS99127F の制御情報に
  ついては上記の LM78mon の作者である 米谷紀幸(:p araffin.)さんに教えて
  いただきました。以上の方々に感謝いたします。

・次の公開 version 1.06 では、最近研究室で計算速度重視のために増えて
  きている Athlon マシンでも使えるようにとの目的で開発を行ないました。
  この時 VIA のチップである VT82C686A/B についてはハードウエアモニタ
  機能は公開されておらず、上記の :p araffin.(米谷)さんに全面的に情報
  を教えていただきました。また、ASUS の多くのマザーボードは VT82C686
  を使っていても、ハードウエアモニタは自社の AS99127F を使っており、
  この時には(当時の FreeBSD 3.X では viapm driver が使えないので) SMBus
  に直接アクセスする必要があります。このための PCI Configuration や
  SMBus への直接アクセス方法などの技術情報の詳細についても、上記の
  :p araffin.(米谷)さんの全面的な御協力を得ました。この御協力がなけ
  ればこのような形のヴァージョンアップ版は作れなかったでしょう。
  いろいろと教えていただいたことには大変感謝しています。

・前回の version 1.07 では、mbmon を利用してハードウエアモニタの
  データを独自に加工して使いたい方の希望を取り入れて、mbmon の
  出力機能を強化しました。これは、[FreeBSD-users-jp 64929] の森田さん
  (morita@cybird.co.jp)のパッチを取り込んだものです。どうもありがとう
  ございました。

・前回の version 1.07 で、いくつかのハードウエアモニタチップと SMBus
  インターフェイスへの対応を拡大しましたが、この情報は Linux 用の
  lm_sensors Project による lm_sensors-2.6.2 のソースコードから
  得ました。このような有用なコードを公開しておられる The Lm_sensors
  Group (http://www.lm-sensors.nu/) に感謝いたします。

・Linux の場合には対応する Cの関数がないために、サポートされない機能が
  ありました(mbmon の強化された出力)。Michael Lenzen さんからこれを全面
  支援する Linux 用の関数をいただきました。version 2.00 からこのパッチ
  を利用して、Linux でも FreeBSD と全く同じ動作が可能になりました。
  どうもありがとうございました。

・version 2.00 の開発の契機となったのは 長村＠道星 さんの LM80 用の
  コードの contribution と励ましのお言葉でした。前回の verion 1.07 
  までは、もともと他のハードウエアモニタチップをサポートすることを考慮
  せずに作ったために、新しい LM80 のコードを組み込むことが難しく、
  長村＠道星 さんがモジュール化の見本となるプログラムを書いてください
  ました。これを元にこれまでサポートしてきたモニタチップそれぞれを
  各モジュールに分離することから始めました。この長村さんの見本がなけ
  ればコードの大改革は行なうことができなかったと思います。このことには
  大変感謝しています。

・version 2.00 では、懸案の AMD756/766/768 と ALi M1535D+ の SMBus IO
  の実装を行ないました。このために再び :p araffin.(米谷)さんに全面的な
  協力お願いしました。長い間色々と教えていただき、ほんとにどうも
  ありがとうございました。

・それから、lm_sensor project (現在の最新 version は 2.6.4 ですが)は
  常に色々なモニタチップや SMBus アクセスの情報を教えてくれるいい情報源
  でした。感謝します。

・Tyan TigerMP/MPX の 2番目のセンサーの取扱いと、温度センサーの初期化
  については、MBM の作者の Alex van Kaam さんと Tamas Miklos さんに
  教えていただきました。どうもありがとうございます。
  
・また、version 2.00 では何人かの方にベータテスターをお願いしました。
  Tom Dean さん、 Julian Elischer さん、藤本＠電大 さん、
  中村和志＠神戸 さん、どうもありがとうございました。

・この version 2.01 では、mbmon をデーモンモードで起動するパッチを
  Jean-Marc Zucconi さん(jmz@FreeBSD.or)からいただきました。何台もの
  マシンの管理をされている人には便利な機能でしょう。
  どうもありがとうございました。

・ASB100 の温度読み出しの改善と ASUS Mozart-2 センサーのサポートは
  MBM の Alex van Kaam さんから教えていただいた情報によるものです。
  いつもどうもありがとうございます。

・xmbmon の NFS ~/.Xauthority ファイル問題の修正は中村和志＠神戸さんに
  送っていただいたパッチによるものです。どうもありがとうございました。

・いつものように、新しい AMD8111 及び NVidia nForce2 チップセットの
  SMBus アクセスの方法を教えていただいた Alex van Kaam 氏には非常に
  感謝しています。


<< 10. 最後に>>

　このプログラムは完全にフリーソフトです。どのように改変・変更しても
かまいませんし、コピー・配布も自由です。また、このプログラムを使ったため
に何か不都合なことが起きても作者は一切の責任を負いません。今のところ、
このプログラムをより一層発展させるつもりはありません。ただ、もしこの
プログラムを元によりよいものができた時には作者にフィードバックしていた
だければ嬉しいです(もちろん全然義務ではありません)。

   2003年7月31日　九州大学大学院理学研究院物理学部門  清水良文

   e-mail:yrsh2scp@mbox.nc.kyushu-u.ac.jp
   http://www.nt.phys.kyushu-u.ac.jp/shimizu/index.html
