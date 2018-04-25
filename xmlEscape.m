function out = xmlEscape(str_in)
%xmlEscape Escape special XML characters in string
%Using thoes described in https://www.ibm.com/support/knowledgecenter/en/SSAW57_8.5.5/com.ibm.websphere.wlp.nd.doc/ae/rwlp_xml_escape.html

out = str_in;

out = strrep(out, '&', '&amp;');
out = strrep(out, '"', '&quot;');
out = strrep(out, '''', '&apos;');
out = strrep(out, '<', '&lt;');
out = strrep(out, '>', '&gt;');

end

