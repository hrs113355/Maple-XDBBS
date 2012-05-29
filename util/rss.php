#!/usr/local/bin/php -q
<?
/****************************************************
  Maple-xdbbs RSS 訂閱器- RSS/ATOM 下載/解讀/配信   
 ****************************************************
 作者: albb0920.bbs  <AT> xdbbs.twbbs.org 
       hrs113355.bbs <AT> xdbbs.twbbs.org
 運行需求: PHP 5.1 以上
           無法於 safe_mode 下運作

 本檔案必須存為 Big5 編碼，並以 bbs 權限執行
 *****************************************************/

define(SHORTURL_API, 'http://loli.tw/apiadd.php?url=');
define(SITEHOST,'xdbbs.twbbs.org');
define(RSSNICK,'叉滴小站 RSS 訂閱器');
// 下載失敗時嘗試以 Proxy 下載
// 主要解決 domain 含底線"_"這個非法字元時
// 部分 OS 拒絕解析的問題
define(ALT_Proxy,'tcp://proxy.edu.tw:3128'); 
define(FOOTER,
"--
※ 發信站: 叉滴小站(XDBBS.twbbs.org)
\x1B[1;30m◆ 作者: XDBBS RSS Reader\x1B[m\n");
define(RSSPATH,'/home/bbs/etc/rss/');
/*------ 以上為設定 ------*/
define(FLAG_OUTGO,      1);
define(FLAG_GET_UPDATE, 2);
define(FLAG_LABEL,      4);
define(FLAG_SKIP,       8);
  /* 檢查設定檔 */
  if(!file_exists(RSSPATH.$argv[1]))
      die('設定檔: '.RSSPATH.$argv[1].' 不存在 EXIT');

  $url = str_replace('#%slash%#','/',$argv[1]);
//  $lastLog=microtime(true); // For performance test,you also need to edit vlog() to enable
  vlog('==> 開始抓取 '.$url);
  $rss = new DOMDocument();
  if (!$rss->load($url))
  {
      $context=stream_context_create(array('http'=>array('proxy'=>ALT_Proxy)));
      $data=file_get_contents($url,0,$context);
      if($data===FALSE)
	  die("下載失敗\n"); // todo: Maybe need to mark rss as failed
      else{
	  $rss->loadXML($data);
	  unset($data);
      }
  }
  vlog('*** OK');

  /* 讀入設定檔 */
  $fp = fopen(RSSPATH.$argv[1],'r+');
  fseek($fp,192);
  fputs($fp,pack('i',time())); // 實際更新時間，僅供介面顯示用
  fseek($fp,200);
  $subscriber=array();  
// typedef struct  必須與 struct.h 中定義相同
//   0~12  char brdname[13+1];
//  13~25  char owner[13+1];
//  26~41  char prefix[16];      
//  42~113 char white[72];       
// 114~185 char black[72];       
// 186~187 char pad2[2];
// 188~191 time_t chrono;
// 192~195 time_t update;
// 196~199 usint attr;
//  } rssbrdlist_t; // 200 bytes
  while(($buf=fread($fp,200))!=''){ // feof() 需要一次抓不到東西的fread，所以以此取代
      $sub=unpack('a13brd/a13owner/a16perfix/a72wfilter/a72bfilter/@192/iupdate/Iattr',$buf);
      // 字串切到 \0
      stripnull($sub['brd']);
      stripnull($sub['owner']);
      stripnull($sub['perfix']);
      stripnull($sub['wfilter']);
      stripnull($sub['bfilter']);
      array_push($subscriber,$sub);
      if(!isset($lastFetch))
	  $lastFetch=$sub['update'];
      elseif($sub['update']<$lastFetch)
	  $lastFetch=$sub['update'];
  }
  unset($buf);
  fclose($fp);
  $latest_time = $lastFetch; // 存最新文章，因為有些站台時間不照順序，亂跳一通
  dl('maplebbs3.so');
  $rssobj = $rss->getElementsByTagName('item');
  if($rssobj->length==0){ // 判斷是不是 atom feed
      $rssobj = $rss->getElementsByTagName('entry');
      if($rssobj->length==0)
	  die('*** 異常中止! 不支援的 Feed 格式 ***');
      else{
	  $IsATOM = true;
	  if($rss->getElementsByTagName('feed')->item(0)->getAttribute('version')=='0.3')
	      $IsOldSpec = true; // 向下支援 ATOM 0.3 (非標準，已廢棄)
      }
  }

  $channel=str_replace(chr(10),' ',iconv('UTF-8','Big5//IGNORE',
      $rss->getElementsByTagName(($IsATOM)?'feed':'channel')->item(0)
      ->getElementsByTagName('title')->item(0)->nodeValue));
  for($i = $rssobj->length - 1;$i>=0;--$i) // 從最舊的 RRS(通常在下面) 往新的掃
  {
      $entry   = $rssobj->item($i);
      // TODO: Maybe more timezone handle
      $pubDate = $entry->getElementsByTagName(($IsATOM)?($IsOldSpec)?'issued':'published':'pubDate');
      $pubDate = ($pubDate->length)?date_create($pubDate->item(0)->nodeValue)->format('U'):0;
      $upDate  = $entry->getElementsByTagName(($IsATOM)?($IsOldSpec)?'modified':'updated':'lastBuildDate');
      $upDate = ($upDate->length)?date_create($upDate->item(0)->nodeValue)->format('U'):0;
      if($pubDate < $lastFetch && $upDate < $lastFetch)
	  continue; // 抓過了
      vlog('文章時間: '.$pubDate.' update:'.$upDate.' > '.$lastFetch);
      $title = $entry->getElementsByTagName('title');
      $title = ($title->length)?
	  iconv('UTF-8','Big5//IGNORE',html_entity_decode($title->item(0)->nodeValue,ENT_QUOTES,'UTF-8')):'';
      $description=$entry->getElementsByTagName(($IsATOM)?'content':'encoded');
      if(!$IsATOM && !$description->length)
	  $description = $entry->getElementsByTagName('description');
      if($description->length)
	  if($IsATOM && $description->item(0)->getAttribute('src')!='')
	      $description = shorturl('詳見: ',$description->item(0)->getAttribute('src'));
	  elseif($IsATOM && $description->item(0)->getAttribute('type')=='html')
	      $description = format($description->item(0)->nodeValue,1);
	  else
	      $description = format($description->item(0)->nodeValue);
      elseif($IsATOM && $entry->getElementsByTagName('summary')->length)
	  $description =  format($entry->getElementsByTagName('summary')->item(0)->nodeValue);
      else
	  $description = $title;
      vlog('@-> Stage1 Done '.$title);

      // 重建 $description ，方便自動斷行，直接把不因看板不同的 Header 寫入
      $copy=&$description; 
      unset($description);
      $description='標題: '.substr($title,0,70).chr(10).
                   '發信站: '.strftime('%D %a %X').chr(10).
                   '轉信: '.RSSNICK.chr(10).
		   chr(10);
      if($pubDate)
	  $description.="\x1B[1;30m發佈時間: \x1B[;1m".strftime('%D %a %X',$pubDate)."\x1B[m    ";
      if($upDate)
	  $description.="\x1B[1;30m修改時間: \x1B[;1m".strftime('%D %a %X',$upDate)."\x1B[m";
      $link=$entry->getElementsByTagName('link');
      if(strlen($title)>70)
	  $description.=chr(10)."\x1B[1;30m完整標題:\x1B[m\n\x1B[1m".graceful_cut($title,79,1,"\x1B[m\n\x1B[1m")."\x1B[m";
      if($link->length){
	  if($IsATOM)
	      $link = $link->item(0)->getAttribute('href');
	  else
	      $link = $link->item(0)->nodeValue;
	  $description.=chr(10).shorturl("\x1B[1;30m原文鏈結:\x1B[;1m ",iconv('UTF-8','Big5//IGNORE',$link),true)."\x1B[m";
      }
      $description.=chr(10).
		    '───────────────────────────────────────'.chr(10);
      $now=0;
      $len=strlen($copy);
      vlog('++> 自動斷行 START');
      while($now<$len){
	  $next=strpos($copy,chr(10),$now);
	  if($next===false)
	      $next=$len;
	  if($next-$now>80){ // > 80 過長，實際上希望 70~80 之間斷
	      for($next=$now+78;$next>$now+70;$next--)
		  if(in_array($copy[$next],array(' ','.',',')))
		      break;
	      if($next==$now+70){ // 沒找到適合的切斷點， TODO:寫法待檢討
		  for($next=$now;$next-$now<78;$next++)
		      if(ord($copy[$next])>128){
			  $next++;//跳過低位元
    			  if($next-$now>70)
			      if(ord($copy[$next+1])<128){ //提前換行
				  $next++;
				  break;}
			      }
		      $next--;
	      }
	      $description.=substr($copy,$now,$next-$now+1).chr(10);
	  }else
	      $description.=substr($copy,$now,$next-$now).chr(10);
	  $now=$next+1;
      }
      unset($copy);
      vlog('**> 過長處理 DONE');
      // 以下是可以讀，但是沒解讀的，
      // 理由是大部分 Feed 提供的這些資料對我們沒參考性
      // rss/atom category , rss comments
      $description .= chr(10);
      if($entry->getElementsByTagName('author')->length){
	  $description.="\x1B[1;30m作者:\x1B[;1m ";
	  $author = $entry->getElementsByTagName('author')->item(0);
	  if($IsATOM){
	      $description.=iconv('UTF-8','Big5//IGNORE',
		  $author->getElementsByTagName('name')->item(0)->nodeValue).chr(27).'[m'.chr(10);
	      if( $author->getElementsByTagName('email')->length )
		  $description.=chr(27).'[1m'.'      '.iconv('UTF-8','Big5//IGNORE',$author->getElementsByTagName('email')->item(0)->nodeValue).chr(27).'[m'.chr(10);
	      if($author->getElementsByTagName('uri')->length)
		  $description.=chr(27).'[1m'.shorturl('      ',$author->getElementsByTagName('uri')->item(0)->nodeValue).chr(27).'[m'.chr(10);
	  }else
	      $description.=iconv('UTF-8','Big5//IGNORE',$author->nodeValue).chr(10);
      }


      $description.="\x1B[1;30m".shorturl("來源:\x1B[;1m ",$url)."\x1B[m".chr(10).
	            "\x1B[1;30m類型: \x1B[;1m".(($IsATOM)?'Atom Feed':'RSS Feed')."\x1B[m".chr(10).
		    FOOTER;
      
      /* 配信 */
      foreach($subscriber as &$sub){
	  if($sub['brd']=='' || $sub['owner']=='' || $sub['attr'] & FLAG_SKIP)
	      continue;
	  if($sub['update'] > $pubDate){
	      if($sub['update'] > $upDate)
		continue;
	      if(!($sub['attr'] & FLAG_GET_UPDATE) && $pubDate > 0)
		  continue;
	  }
	  vlog('brd->'.$sub['brd']);
	  /* filter */
	  if($sub['wfilter']!=''){  // 白名單
	      $filter=explode('/',$sub['wfilter']);
	      $ubound=count($filter);
	      for($p=0;$p<=$ubound;$p++)
		  if($filter[$p]!='' && stripos($title,$filter[$p])!==FALSE)
		      break;
	      if($p>$ubound)
		  continue;
	  }
	  if($sub['bfilter']!=''){ // 黑名單
	      $filter=explode('/',$sub['bfilter']);
	      $ubound=count($filter);
	      for($p=0;$p<=$ubound;$p++)
		  if($filter[$p]!='' && stripos($title,$filter[$p])!==FALSE)
		      break;
	      if($p<=$ubound)
		  continue;
	  }

	  doPost($sub['brd'],
	      ($sub['attr'] & FLAG_LABEL)?($title[0]=='[')?'['.$sub['perfix'].']'.$title:
	      '['.$sub['perfix'].'] '.$title:$title,
	      $sub['owner'].'.rss@'.SITEHOST,$channel,
	      '發信人: '.$channel.' 看板: '.$sub['brd'].chr(10).$description,$sub['attr'] & FLAG_OUTGO);
	  // doPost 的第二個參數是標題，對於不處理中文斷字的系統(ex:M3-itoc原廠)
	  // 建議丟到 graceful_cut()去，第二個參數給 45(maybe 44)
	  vlog('  > '.'OK');
      }
      unset($sub); // clean up 
      if($upDate>$pubDate)
	  $pubDate=$upDate;
      if(++$pubDate>$latest_time)    
	  $latest_time=$pubDate;
  }
  /* 避免來自未來的 RSS 發生，造成一抓再抓，標記 lastFech 為最新RSS時間 */
  if($latest_time>$lastFetch){
      $latest_time=pack('i',$latest_time); // 換成二進位
      /* 把 lastFetch 寫給所以人 */
      $fp = fopen(RSSPATH.$argv[1],'r+');
      $ubound=count($subscriber);
      for($p=1;$p<=$ubound;++$p){
          fseek($fp,200*$p+192); 
          fputs($fp,$latest_time);
      }
      fclose($fp);
  }
  vlog('** ALL DONE **');
/* 把 HTML/純文字 處理搬過來 */
  function format(&$str,$knownHTML=0){
      //todo: 待改善
      $str=html_entity_decode($str,ENT_COMPAT,'UTF-8');
      if($knownHTML || stripos($str,'<br')!==FALSE || stripos($str,'<p')!==FALSE){ // HTML
	  $str=preg_replace_callback('% +|\n\r|\n|\r|<pre>.*?</pre>%si', 'format_callback', $str);
	  // 處理換行
	  //  TODO:
          // 0. ATOM feed 支援純文字
          // 1. <p></p> 詳細判定
	  $str = str_ireplace(array('<br>','<br/>','<br />','<p>','</p>'),array(chr(10),chr(10),chr(10),chr(10),chr(10)),$str);
	  vlog(' -> 換行處理 Done ');
	  $str=iconv('UTF-8','Big5//IGNORE',$str); // 拖到現在才做，應該會快一點吧 Orz
          // 處理圖片
	  $str=preg_replace_callback('/\n?<img.*?src="(.*?)".*?>\n?/i','img_callback',$str);
           vlog(' -> 圖片處理 Done ');
          // 處理鏈結
          $str=preg_replace_callback('/[\n<\[\( ]*<a.*?href="(.*?)".*?>(.*?)<\/a>[>\]\)\n ]*/i','ahref_callback',$str);
          vlog(' -> 鏈結處理 Done ');
          // 其他標籤
	  $str=strip_tags($str);
	  return $str;
      }else{ // 純文字
	  $str=iconv('UTF-8','Big5//IGNORE',$str);
	  // 還是無法保證完全沒有 HTML，所以各自搜尋
	  if(stripos($str,'<img')!==FALSE)
	      $str=preg_replace_callback('/\n?<img.*?src="(.*?)".*?>\n?/i','img_callback',$str);
	  if(stripos($str,'<a')!==FALSE)
	      $str=preg_replace_callback('/[\n<\[\( ]*<a.*?href="(.*?)".*?>(.*?)<\/a>[>\]\)\n ]*/i','ahref_callback',$str);
	  else
	      $str=preg_replace_callback('%[a-zA-Z0-9]*?://[^ "<>\[\]\^`\{\}\n]+%','texturl',$str);// 硬抓出鏈結      
	  return $str;
      }
  }

  /* callbacks */
  function format_callback($matches){
    switch($matches[0][0]){
    case ' ':
	return ' ';
    case chr(10):
    case chr(13):
	return '';
    case '<':
	return $matches[0];
    }
  }
  function img_callback($matchs){
	return chr(10).shorturl('[圖片] ',$matchs[1]).chr(10);
  }
  function texturl($matchs){
    return chr(10).shorturl('[鏈結] ',$matchs[0]).chr(10);
  }
  function ahref_callback($matchs){
      $matchs[2]=strip_tags($matchs[2]);
      if( $matchs[2]==$matchs[1]||  
	  substr($matchs[2],0,7)=='http://'|| 
	  substr($matchs[2],0,8)=='https://'||
	  substr($matchs[2],0,6)=='ftp://'||
	  substr($matchs[2],0,9)=='telnet://'||
	  substr($matchs[2],0,6)=='mms://'||
	  substr($matchs[2],0,7)=='fd2k://'||
	  substr($matchs[2],0,7)=='rtsp://'||
	  substr($matchs[2],0,7)=='mailto:')
	  return chr(10).shorturl('[鏈結] ',$matchs[1]).chr(10);
      else{
	  if(strlen($matchs[2])>51)
	      $matchs[2]=graceful_cut($matchs[2],49).'…';
	  return chr(10).shorturl('[鏈結:"'.$matchs[2].'"] ',$matchs[1]).chr(10);
      
      }
  }
  /* Utilities */
  function stripnull(&$str){
      if(($pos=strpos($str,chr(0)))!==FALSE)
	  $str=substr($str,0,$pos);
  }
  function shorturl($prompt,$url,$oneline=false){
      if(stripos($url,':/')==false&&substr($matchs[4],0,7)!='mailto:'){
	  if($url[0]=='/'){ // TODO:其實應該判斷更多的，但是很麻煩，先這樣
	    global $entry,$IsATOM,$rss;
	    if(($base=$entry->getAttribute('base'))=='')
		$base=$rss->getElementsByTagName(($IsATOM)?'feed':'channel')->item(0)->
		    getElementsByTagName('link')->item(0)->getAttribute('href');
		$url=substr($base,0,strpos($base,'/',8)).$url;
	  }
	  else
	      $url='http://'.$url;
      }
      $len = strlen($url);
      if( $len>80 || ($oneline && $len>79-strlen($prompt) ) )
	  return $prompt.file_get_contents(SHORTURL_API.urlencode($url));
      elseif($len>79-strlen($prompt))
	     return $prompt.chr(10).$url;
      else
	  return $prompt.$url;
  }
  function vlog($msg){
/*    Uncomment these if you need performance test.
      global $lastLog;
      $now=microtime(true);
      echo sprintf('%05.2f',$now-$lastLog),' ',$msg,chr(10);
      $lastLog=$now;*/
      echo $msg,chr(10);
  }
  function graceful_cut($str,$len,$recursive=0,$between="\n"){
      if(strlen($str)<$len)
	  return $str;
      for($ptr=0;$ptr<$len;$ptr++){
	  if(ord($str[$ptr])>128)
	      if($ptr==$len-1)
		  break;
	      else
		  $ptr++;
      }
      if($recursive && $len < strlen($str))
	  return substr($str,0,$ptr).$between.graceful_cut(substr($str,$ptr),$len,1);
      else
       	  return substr($str,0,$ptr);
  }

?>
