function  writeNTabs(file,nTabs)
%writeNTabs Write N tabs into a file

for i = 1:nTabs
   fprintf(file, '\t');
end

end

