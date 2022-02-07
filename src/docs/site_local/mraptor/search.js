<!--  Copyright 2000 KScripts Web Consulting and Design. All rights reserved. Please do not copy without consent -->
<!-- Created April 2000 by David E. Kim -- KScripts Web Consulting and Design -- www.kscripts.com -->

<!-- // START OF SEARCH SCRIPT

// START CONFIGURATION	(Do not edit this line)
var displaymatches = 10; // number of matches to display
var termlength = 2; // minimum term length
var displaylastupdated = 1; // set to 0 if you don't want the last updated date to be shown in results
var displayfilesize = 1; // set to 0 if you don't want the file size to be shown in results
var urlprefix = "./";    // where the manual resides
var pageref = "index.html.page="; // the string to append to urlprefix when viewing non-index.html pages
var style_sheet = "style.css"; // url of style sheet
// END CONFIGURATION	(Do not edit this line)

function results (wantedpage) {
	if (parent.opener) {
		parent.opener.document.location.href = wantedpage;
		parent.opener.focus();
	} else {
		var windowsettings = "toolbar=yes,location=yes,directories=yes, status=yes,menubar=yes,scrollbars=yes,resizable=yes,copyhistory=yes";
		var newWindow = window.open(wantedpage,"New",windowsettings);
		newWindow.focus();
	}
}
var other, minus, plus;
var currentmatch = 0;
//var mainframe = parent.frames[1].document;
var mainBody = document.getElementById("mainBody");
var newContent = "";
var sortedurls = new Array();
var cleanquery = new Array();

function search (q) {
	var notmatching = ""; // initialize variables
	other = "";
	minus = "";
	plus = "";
	q = q.replace(/\s+/g, " "); // remove extra spaces
	q = q.replace(/^\s+/g, ""); 
	q = q.replace(/\s+$/g, "");
	q = q.toLowerCase();	// switch to lower case
	var noplusminus = q.replace(/^[\-\+]/,"");
	if (noplusminus.length < termlength) {	// check term length
		alert("Term must be at least " + termlength + " characters long");
		document.searchform.query.focus();
		return;
	}
	var done = "";
	var isitanumber = /^[0-9]+$/;		// number regex
	cleanquery = clean(q.split(" "));	// remove repeating terms
	for (i=0;i<cleanquery.length;i++) {	// process each term
		var cleaned = cleanquery[i];
		if (isitanumber.test(cleaned)) {	// add # in front of numbers
			cleaned = "#" + cleaned;
		}
		if (cleanquery[i].charAt(0) == "+" && cleanquery[i].length > 1) {	// get plus boolean terms
			cleaned = cleanquery[i].replace(/^\+/,"");
			if (isitanumber.test(cleaned)) {
				cleaned = "#" + cleaned;
			}
			if (w[cleaned]) {
				plus += " " + w[cleaned];
			} else {
				notmatching += " " + cleanquery[i];
			}
			continue;
		}
		if (cleanquery[i].charAt(0) == "-" && cleanquery[i].length > 1) { // get minus boolean terms
			cleaned = cleanquery[i].replace(/^\-/,"");
			if (isitanumber.test(cleaned)) {
				cleaned = "#" + cleaned;
			}
			if (w[cleaned]) {
				minus += "|" + w[cleaned];
			} 
			continue;
		}
		if (w[cleaned]) {
			other += " " + w[cleaned];
		} else {
			notmatching += " " + cleanquery[i];
		}
	}
	document.searchform.query.value=cleanquery.join(" ");
	if (notmatching) {
		notmatching = notmatching.replace(/^\s+/, "");
		notmatching = notmatching.split(" ");
	}
	process_booleans(notmatching);
}

function process_booleans(notmatching) {
	var a = "";
	var allurlstrings = "";
	var nextinloop = "";
	if (notmatching.length >= 1) { noresults(notmatching); return; }
	if (other) {
		other = other.replace(/^\s+/, "");
		other = other.split(" ");
		for (j=0;j<other.length;j++) {	// process other terms
			var urlstrings = other[j].split("|"); 
			for (k=0;k<urlstrings.length;k++) {
				allurlstrings += " " + urlstrings[k]; 
			}
		}
	}
	if (plus) {
		plus = plus.replace(/^\s+/, "");
		plus = plus.split(" ");
		for (j=0;j<plus.length;j++) {	// process plus boolean terms
			var turlslist = "";
			var urlslist = "";
			var urlstrings = plus[j].split("|"); 
			for (k=0;k<urlstrings.length;k++) {
				allurlstrings += " " + urlstrings[k]; 
				var breakup = urlstrings[k].split("=");
				urlslist += " " + breakup[0];
			}
			urlslist = urlslist.replace(/^\s+/,"");
			urlslist = urlslist.split(" ");
			if (!nextinloop) { var allwordsurls = urlslist; nextinloop = 1; continue;} 
			for (l=0;l<urlslist.length;l++) {
				for (m=0;m<allwordsurls.length;m++) {
					if (urlslist[l] == allwordsurls[m]) {
						turlslist += " " + urlslist[l];
					}
				}
			}
			turlslist = turlslist.replace(/^\s+/,"");
			turlslist = turlslist.split(" ");
			var allwordsurls = turlslist; 
		}
		allurlstrings = allurlstrings.replace(/^\s+/,"");
		allurlstrings = allurlstrings.split(" ");
		for (n=0;n<allurlstrings.length;n++) {
			var breakup = allurlstrings[n].split("=");
			for (o=0;o<allwordsurls.length;o++) {
				if (breakup[0] == allwordsurls[o]) {
					a += " " + allurlstrings[n];
				}
			}
		}
	} else {
		a = allurlstrings;
	}
	if (minus) {	// process minus booleans
		var atemp = "";
		minus = minus.replace(/^\|/,"");
		minus = minus.split("|");
		var minusurls = geturls(minus);
		a = a.replace(/^\s+/,"");
		a = a.split(" ");
		outerloop:
		for (n=0;n<a.length;n++) {
			var breakup = a[n].split("=");
			for (o=0;o<minusurls.length;o++) {
				if (breakup[0] == minusurls[o]) {
					continue outerloop;
				}
			}	
			atemp += " " + a[n];
		}
		a = atemp;
	}
	a = a.replace(/^\s+/,"");
	a = a.split(" ");
	a = a.join("|");
	if (a) {
		process(a);
	} else {
		noresults(cleanquery);  
	}
}
function noresults (q) {
	var noresults = "";
	var newcleanquery = "";
	for (h=0;h<cleanquery.length;h++) {
		var match = "";
		for (g=0;g<q.length;g++) {
			if (cleanquery[h] == q[g]) {
				match = 1;
			}
		}
		if (!match) {newcleanquery += " " + cleanquery[h];}
	}	
	newcleanquery = newcleanquery.replace(/^\s+/,"");
	noresults = q.join(" ");
	if (q.length == 1) { 
		if (newcleanquery) {
			var finalnoresults = q[0] + "  \nContinuing without this term";
			alert ("Cannot find: " + finalnoresults);		
		} else {
			var finalnoresults = q[0];
			alert ("No documents match: " + finalnoresults);		
		}
	}
	if (q.length > 1) {
		if (newcleanquery) {
			noresults += "  \nContinuing without these terms";
			alert ("Cannot find: " + noresults);
		} else {
			alert ("No documents match: " + noresults);
		}
	}
	if (newcleanquery) {
		search(newcleanquery);
	}
	document.searchform.query.focus();
	document.searchform.query.select();
	return;
}
function process (a) {
	var sorturls = "";
	var urls = new Array();
	var query = a.split("|");
	urls = geturls(query);
	for (j=0;j<urls.length;j++) {
		var total = new Number();
		var n = 0; 
		for (k=0;k<query.length;k++) {
			urlinfo = query[k].split("=");
			if (urlinfo[0] != urls[j]) { continue;}
			n = parseFloat(urlinfo[1]); 
			total += n;
		}
		sorturls += " " + (total + "|" + urls[j]);
	}
	var arethereurls = sorturls;
	sorturls = sorturls.replace(/^\s+/,"");
	sorturls = sorturls.split(" ");
	sortedurls = sorturls.sort(bynumber);
	if (!arethereurls) { 
		noresults(cleanquery); 
	} else {
		display(sortedurls, currentmatch, displaymatches);
	}
}		
function bynumber(a,b) { 
	c = a.split("|");
	d = b.split("|");
	return d[0] - c[0]
}
function clean(q) {
	var s = "";
	var c = new Array();
	var t = new Array();
	for (i=0;i<q.length;i++) {
		if (!t[q[i]]) { s += " " + q[i] ; t[q[i]] = 1;}
	}
	s = s.replace(/^[\s]+/, "");
	c = s.split(" ");
	return c;
}
function geturls (query) {
	var urls = ""; 
	var t = new Array();
	for (z=0;z<query.length;z++) {
		urlinfo = query[z].split("=");
		if (!t[urlinfo[0]]) { urls += " " + urlinfo[0]; t[urlinfo[0]] = 1;}
	}
	urls = urls.replace(/^\s+/,"");
	urls = urls.split(" ");
	return urls;
}
function display(urls, ref, dif) {
	var displaynumber = (urls.length < ref + dif ? urls.length : ref + dif);
	// mainframe = parent.frames[1].document;
        mainBody = document.getElementById("mainBody");
	//mainframe.open();
        newContent = "";
	if (urls.length == 0) {
		newContent = newContent + '<h3>Lost Search Data</h3>\n' + 
		'<table width=95% border=0 align=center cellpadding=3 cellspacing=0><tr><td align=center nowrap>' + 
		'<BR><BR><P><B>Lost Data. Please Resubmit Your Query.</B></P>' + 
		'<P><B>(Click the Search Button)</B></P></td></tr></table>';
                mainBody.innerHTML = newContent;
		return;
	}
	newContent = newContent + 
	'<table width=95% border=0 align=center cellpadding=5 cellspacing=0>\n<tr><td class="results">\n' + 
        '<table width=95% border="0" cellpadding="0" cellspacing="0"><tr><td>' +
	'<B>Search Terms: ' + document.searchform.query.value + 
	'<BR>';
	if (ref + 1 == displaynumber) {
		newContent = newContent + 'Results: ' + displaynumber + ' of ' + urls.length;
	} else {
		newContent = newContent + 'Results: ' + (ref + 1) + ' - ' + displaynumber + ' of ' + urls.length;
	}
        newContent = newContent + '<br><a class="search_results" href="' + urlprefix + pageref + 'search.help">Search Tips</a>';
	newContent = newContent + '</B></td><td class="results" align=right nowrap>';
	prevnext(urls.length, ref, dif, "top");
	newContent = newContent + '</td></tr></table></td></tr>\n' + '\n<!-- // start search results list // -->\n\n';	
	for (var i=ref;i<displaynumber;i++) {
		searchresults = urls[i].split("|");
		urlinfo = u[searchresults[1]];
		temparray = urlinfo.split("|");
                tempurlprefix = urlprefix;
                if (temparray[0] != "index.html") {
                  tempurlprefix = tempurlprefix + pageref;
                  temparray[0] = temparray[0].replace(/\.html/,"");
                }
		newContent = newContent + '<tr><td class="results">\n';
		newContent = newContent + '	<DL><DT><B>' + (i+1) + '.</B> &nbsp; <a href="' + tempurlprefix + 
		temparray[0] + '" class="search_results">' + temparray[1] + '</a>\n';
		newContent = newContent + '		<DD>' + temparray[2] + '...\n';
		newContent = newContent + '		<DD><a href="' + tempurlprefix + temparray[0] + 
		'" class="search_results"><font size=1>' 
		+ tempurlprefix + temparray[0] + '</font></a>\n';
		newContent = newContent + '		<DD><font size=1><B>';
		if (searchresults[0] == "1" && searchresults[0] != "0") { newContent = newContent + '		' + searchresults[0] + ' Match ';}
		if (searchresults[0] != "1" && searchresults[0] != "0") { newContent = newContent + '		' + searchresults[0] + ' Matches '; }
		if (displayfilesize) { newContent = newContent + '		&nbsp; Size: ' + temparray[3] + ' kb ';}
		if (displaylastupdated) { newContent = newContent + '		&nbsp; Last Updated: ' + temparray[4];}
		newContent = newContent + '		</B></font>\n	</DT></DL>\n</td></tr>\n';
	}
	newContent = newContent + '<tr><td class="results" nowrap><P>';
	if (urls.length - ref > 4) {
		prevnext(urls.length, ref, dif, "bottom");
	}
	newContent = newContent + '</P></td></tr>';
	newContent = newContent + '<tr><td>&nbsp;</td></tr><tr><td class="copyright"><center><font size=1>KSearch Client Side by kscripts.com<br>KScripts.com  &copy; Copyright 2000, All rights reserved</font></center></td></tr>\n';
	newContent = newContent + '</table>';
        mainBody.innerHTML = newContent;
	//parent.frames[0].document.searchform.query.focus();
	//parent.frames[0].document.searchform.query.select();
}
function prevnext (top, ref , dif, location) {
	if (location == "top") {
		newContent = newContent + '<form>';
	} else {
		newContent = newContent + '<center><form>';
	}
	if (ref > 0) {
		newContent = newContent + '<input type=button value="Previous ' + dif + '" ' + 
		'onClick="display(sortedurls, ' + 
		(ref - dif) + ', ' + dif + ')">';
	}
	if (ref >= 0 && ref + dif < top) {
		var ttop = ((top - (dif + ref) < dif) ? top - (ref + dif) : dif);
		newContent = newContent + ' &nbsp; <input type=button value="Next ' + ttop + '" ' + 
		'onClick="display(sortedurls, ' +
		(ref + dif) + ', ' + dif + ')">';
	}
	if (location == "top") {
		newContent = newContent + '</form>\n';
	} else {
		newContent = newContent + '</form></center>\n';
	}
}
// END OF SEARCH SCRIPT -->

