#!/usr/local/bin/php -q
<?
/****************************************************
  Maple-xdbbs RSS �q�\��- RSS/ATOM �U��/��Ū/�t�H   
 ****************************************************
 �@��: albb0920.bbs  <AT> xdbbs.twbbs.org 
       hrs113355.bbs <AT> xdbbs.twbbs.org
 �B��ݨD: PHP 5.1 �H�W
           �L�k�� safe_mode �U�B�@

 ���ɮץ����s�� Big5 �s�X�A�åH bbs �v������
 *****************************************************/

define(SHORTURL_API, 'http://loli.tw/apiadd.php?url=');
define(SITEHOST,'xdbbs.twbbs.org');
define(RSSNICK,'�e�w�p�� RSS �q�\��');
// �U�����Ѯɹ��եH Proxy �U��
// �D�n�ѨM domain �t���u"_"�o�ӫD�k�r����
// ���� OS �ڵ��ѪR�����D
define(ALT_Proxy,'tcp://proxy.edu.tw:3128'); 
define(FOOTER,
"--
�� �o�H��: �e�w�p��(XDBBS.twbbs.org)
\x1B[1;30m�� �@��: XDBBS RSS Reader\x1B[m\n");
define(RSSPATH,'/home/bbs/etc/rss/');
/*------ �H�W���]�w ------*/
define(FLAG_OUTGO,      1);
define(FLAG_GET_UPDATE, 2);
define(FLAG_LABEL,      4);
define(FLAG_SKIP,       8);
  /* �ˬd�]�w�� */
  if(!file_exists(RSSPATH.$argv[1]))
      die('�]�w��: '.RSSPATH.$argv[1].' ���s�b EXIT');

  $url = str_replace('#%slash%#','/',$argv[1]);
//  $lastLog=microtime(true); // For performance test,you also need to edit vlog() to enable
  vlog('==> �}�l��� '.$url);
  $rss = new DOMDocument();
  if (!$rss->load($url))
  {
      $context=stream_context_create(array('http'=>array('proxy'=>ALT_Proxy)));
      $data=file_get_contents($url,0,$context);
      if($data===FALSE)
	  die("�U������\n"); // todo: Maybe need to mark rss as failed
      else{
	  $rss->loadXML($data);
	  unset($data);
      }
  }
  vlog('*** OK');

  /* Ū�J�]�w�� */
  $fp = fopen(RSSPATH.$argv[1],'r+');
  fseek($fp,192);
  fputs($fp,pack('i',time())); // ��ڧ�s�ɶ��A�ȨѤ�����ܥ�
  fseek($fp,200);
  $subscriber=array();  
// typedef struct  �����P struct.h ���w�q�ۦP
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
  while(($buf=fread($fp,200))!=''){ // feof() �ݭn�@���줣��F�誺fread�A�ҥH�H�����N
      $sub=unpack('a13brd/a13owner/a16perfix/a72wfilter/a72bfilter/@192/iupdate/Iattr',$buf);
      // �r����� \0
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
  $latest_time = $lastFetch; // �s�̷s�峹�A�]�����ǯ��x�ɶ����Ӷ��ǡA�ø��@�q
  dl('maplebbs3.so');
  $rssobj = $rss->getElementsByTagName('item');
  if($rssobj->length==0){ // �P�_�O���O atom feed
      $rssobj = $rss->getElementsByTagName('entry');
      if($rssobj->length==0)
	  die('*** ���`����! ���䴩�� Feed �榡 ***');
      else{
	  $IsATOM = true;
	  if($rss->getElementsByTagName('feed')->item(0)->getAttribute('version')=='0.3')
	      $IsOldSpec = true; // �V�U�䴩 ATOM 0.3 (�D�зǡA�w�o��)
      }
  }

  $channel=str_replace(chr(10),' ',iconv('UTF-8','Big5//IGNORE',
      $rss->getElementsByTagName(($IsATOM)?'feed':'channel')->item(0)
      ->getElementsByTagName('title')->item(0)->nodeValue));
  for($i = $rssobj->length - 1;$i>=0;--$i) // �q���ª� RRS(�q�`�b�U��) ���s����
  {
      $entry   = $rssobj->item($i);
      // TODO: Maybe more timezone handle
      $pubDate = $entry->getElementsByTagName(($IsATOM)?($IsOldSpec)?'issued':'published':'pubDate');
      $pubDate = ($pubDate->length)?date_create($pubDate->item(0)->nodeValue)->format('U'):0;
      $upDate  = $entry->getElementsByTagName(($IsATOM)?($IsOldSpec)?'modified':'updated':'lastBuildDate');
      $upDate = ($upDate->length)?date_create($upDate->item(0)->nodeValue)->format('U'):0;
      if($pubDate < $lastFetch && $upDate < $lastFetch)
	  continue; // ��L�F
      vlog('�峹�ɶ�: '.$pubDate.' update:'.$upDate.' > '.$lastFetch);
      $title = $entry->getElementsByTagName('title');
      $title = ($title->length)?
	  iconv('UTF-8','Big5//IGNORE',html_entity_decode($title->item(0)->nodeValue,ENT_QUOTES,'UTF-8')):'';
      $description=$entry->getElementsByTagName(($IsATOM)?'content':'encoded');
      if(!$IsATOM && !$description->length)
	  $description = $entry->getElementsByTagName('description');
      if($description->length)
	  if($IsATOM && $description->item(0)->getAttribute('src')!='')
	      $description = shorturl('�Ԩ�: ',$description->item(0)->getAttribute('src'));
	  elseif($IsATOM && $description->item(0)->getAttribute('type')=='html')
	      $description = format($description->item(0)->nodeValue,1);
	  else
	      $description = format($description->item(0)->nodeValue);
      elseif($IsATOM && $entry->getElementsByTagName('summary')->length)
	  $description =  format($entry->getElementsByTagName('summary')->item(0)->nodeValue);
      else
	  $description = $title;
      vlog('@-> Stage1 Done '.$title);

      // ���� $description �A��K�۰��_��A�����⤣�]�ݪO���P�� Header �g�J
      $copy=&$description; 
      unset($description);
      $description='���D: '.substr($title,0,70).chr(10).
                   '�o�H��: '.strftime('%D %a %X').chr(10).
                   '��H: '.RSSNICK.chr(10).
		   chr(10);
      if($pubDate)
	  $description.="\x1B[1;30m�o�G�ɶ�: \x1B[;1m".strftime('%D %a %X',$pubDate)."\x1B[m    ";
      if($upDate)
	  $description.="\x1B[1;30m�ק�ɶ�: \x1B[;1m".strftime('%D %a %X',$upDate)."\x1B[m";
      $link=$entry->getElementsByTagName('link');
      if(strlen($title)>70)
	  $description.=chr(10)."\x1B[1;30m������D:\x1B[m\n\x1B[1m".graceful_cut($title,79,1,"\x1B[m\n\x1B[1m")."\x1B[m";
      if($link->length){
	  if($IsATOM)
	      $link = $link->item(0)->getAttribute('href');
	  else
	      $link = $link->item(0)->nodeValue;
	  $description.=chr(10).shorturl("\x1B[1;30m����쵲:\x1B[;1m ",iconv('UTF-8','Big5//IGNORE',$link),true)."\x1B[m";
      }
      $description.=chr(10).
		    '�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w�w'.chr(10);
      $now=0;
      $len=strlen($copy);
      vlog('++> �۰��_�� START');
      while($now<$len){
	  $next=strpos($copy,chr(10),$now);
	  if($next===false)
	      $next=$len;
	  if($next-$now>80){ // > 80 �L���A��ڤW�Ʊ� 70~80 �����_
	      for($next=$now+78;$next>$now+70;$next--)
		  if(in_array($copy[$next],array(' ','.',',')))
		      break;
	      if($next==$now+70){ // �S���A�X�����_�I�A TODO:�g�k���˰Q
		  for($next=$now;$next-$now<78;$next++)
		      if(ord($copy[$next])>128){
			  $next++;//���L�C�줸
    			  if($next-$now>70)
			      if(ord($copy[$next+1])<128){ //���e����
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
      vlog('**> �L���B�z DONE');
      // �H�U�O�i�HŪ�A���O�S��Ū���A
      // �z�ѬO�j���� Feed ���Ѫ��o�Ǹ�ƹ�ڭ̨S�Ѧҩ�
      // rss/atom category , rss comments
      $description .= chr(10);
      if($entry->getElementsByTagName('author')->length){
	  $description.="\x1B[1;30m�@��:\x1B[;1m ";
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


      $description.="\x1B[1;30m".shorturl("�ӷ�:\x1B[;1m ",$url)."\x1B[m".chr(10).
	            "\x1B[1;30m����: \x1B[;1m".(($IsATOM)?'Atom Feed':'RSS Feed')."\x1B[m".chr(10).
		    FOOTER;
      
      /* �t�H */
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
	  if($sub['wfilter']!=''){  // �զW��
	      $filter=explode('/',$sub['wfilter']);
	      $ubound=count($filter);
	      for($p=0;$p<=$ubound;$p++)
		  if($filter[$p]!='' && stripos($title,$filter[$p])!==FALSE)
		      break;
	      if($p>$ubound)
		  continue;
	  }
	  if($sub['bfilter']!=''){ // �¦W��
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
	      '�o�H�H: '.$channel.' �ݪO: '.$sub['brd'].chr(10).$description,$sub['attr'] & FLAG_OUTGO);
	  // doPost ���ĤG�ӰѼƬO���D�A��󤣳B�z�����_�r���t��(ex:M3-itoc��t)
	  // ��ĳ��� graceful_cut()�h�A�ĤG�ӰѼƵ� 45(maybe 44)
	  vlog('  > '.'OK');
      }
      unset($sub); // clean up 
      if($upDate>$pubDate)
	  $pubDate=$upDate;
      if(++$pubDate>$latest_time)    
	  $latest_time=$pubDate;
  }
  /* �קK�Ӧۥ��Ӫ� RSS �o�͡A�y���@��A��A�аO lastFech ���̷sRSS�ɶ� */
  if($latest_time>$lastFetch){
      $latest_time=pack('i',$latest_time); // �����G�i��
      /* �� lastFetch �g���ҥH�H */
      $fp = fopen(RSSPATH.$argv[1],'r+');
      $ubound=count($subscriber);
      for($p=1;$p<=$ubound;++$p){
          fseek($fp,200*$p+192); 
          fputs($fp,$latest_time);
      }
      fclose($fp);
  }
  vlog('** ALL DONE **');
/* �� HTML/�¤�r �B�z�h�L�� */
  function format(&$str,$knownHTML=0){
      //todo: �ݧﵽ
      $str=html_entity_decode($str,ENT_COMPAT,'UTF-8');
      if($knownHTML || stripos($str,'<br')!==FALSE || stripos($str,'<p')!==FALSE){ // HTML
	  $str=preg_replace_callback('% +|\n\r|\n|\r|<pre>.*?</pre>%si', 'format_callback', $str);
	  // �B�z����
	  //  TODO:
          // 0. ATOM feed �䴩�¤�r
          // 1. <p></p> �ԲӧP�w
	  $str = str_ireplace(array('<br>','<br/>','<br />','<p>','</p>'),array(chr(10),chr(10),chr(10),chr(10),chr(10)),$str);
	  vlog(' -> ����B�z Done ');
	  $str=iconv('UTF-8','Big5//IGNORE',$str); // ���{�b�~���A���ӷ|�֤@�I�a Orz
          // �B�z�Ϥ�
	  $str=preg_replace_callback('/\n?<img.*?src="(.*?)".*?>\n?/i','img_callback',$str);
           vlog(' -> �Ϥ��B�z Done ');
          // �B�z�쵲
          $str=preg_replace_callback('/[\n<\[\( ]*<a.*?href="(.*?)".*?>(.*?)<\/a>[>\]\)\n ]*/i','ahref_callback',$str);
          vlog(' -> �쵲�B�z Done ');
          // ��L����
	  $str=strip_tags($str);
	  return $str;
      }else{ // �¤�r
	  $str=iconv('UTF-8','Big5//IGNORE',$str);
	  // �٬O�L�k�O�ҧ����S�� HTML�A�ҥH�U�۷j�M
	  if(stripos($str,'<img')!==FALSE)
	      $str=preg_replace_callback('/\n?<img.*?src="(.*?)".*?>\n?/i','img_callback',$str);
	  if(stripos($str,'<a')!==FALSE)
	      $str=preg_replace_callback('/[\n<\[\( ]*<a.*?href="(.*?)".*?>(.*?)<\/a>[>\]\)\n ]*/i','ahref_callback',$str);
	  else
	      $str=preg_replace_callback('%[a-zA-Z0-9]*?://[^ "<>\[\]\^`\{\}\n]+%','texturl',$str);// �w��X�쵲      
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
	return chr(10).shorturl('[�Ϥ�] ',$matchs[1]).chr(10);
  }
  function texturl($matchs){
    return chr(10).shorturl('[�쵲] ',$matchs[0]).chr(10);
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
	  return chr(10).shorturl('[�쵲] ',$matchs[1]).chr(10);
      else{
	  if(strlen($matchs[2])>51)
	      $matchs[2]=graceful_cut($matchs[2],49).'�K';
	  return chr(10).shorturl('[�쵲:"'.$matchs[2].'"] ',$matchs[1]).chr(10);
      
      }
  }
  /* Utilities */
  function stripnull(&$str){
      if(($pos=strpos($str,chr(0)))!==FALSE)
	  $str=substr($str,0,$pos);
  }
  function shorturl($prompt,$url,$oneline=false){
      if(stripos($url,':/')==false&&substr($matchs[4],0,7)!='mailto:'){
	  if($url[0]=='/'){ // TODO:������ӧP�_��h���A���O�ܳ·СA���o��
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
