--- recbond Ver1.0.0 ---

これは、recpt1をBonDriver対応させたものです。

BonDriverProxy_LinuxのヘッダーをインクルードしているのでBonDriverProxy_Linuxの
ソースディレクトリ上にrecbondのディレクトリを残したまま展開してビルドしてください。


[チャンネル指定]
BonDriverチャンネル指定に加えいくつかの条件がありますがrecpt1で使われている従来の
ものをそのまま使用できます。
またBS/CSについてはDVB対応のためにTSIDによる指定(16進数か10進数)を新たに加えました。
なおBonDriverチャンネル指定方法は、"Bn"(nは0から始まる数字)となります。

上記にある「いくつかの条件」とは以下の3点です。
・BonDriverでのサービスの絞込み(#USESERVICEID=1)は、想定していませんので無保証です。
・BonDriverのconfファイルは、同梱のものを使用する。
・confファイルとpt1_dev.hのチャンネル定義は並びを含めて同期させてあるためBSで局編成
  の変更があった場合を除き一切変更してはいけない。

LinuxのBonDriverの場合は、同梱のパッチとconfファイルを差し替えることにより全ての
チャンネル指定方法が利用可能です。

WindowsのBonDriverをBonDriverProxy経由で使用する場合は、iniファイルをpt1_dev.hに
同期させる必要があります。sidとTSIDでのチャンネル指定を利用するにはBonDriverの改造
が必要です。


[BonDriver指定]
フルパス指定の他に短縮指定と省略時の自動選択(BonDriverチャンネル指定時は不可)が
行なえます。
短縮指定と自動選択を利用する場合は、ビルド時にpt1_dev.hを編集して各BonDriverの
ファイルパスを登録してください。

短縮指定の指定方法は、地デジが"Tn"・BS/CSは"Sn"(nは0から始まる数字)・BonDriver_Proxy
を利用する場合は"P"をそれぞれの頭に付加してください。

