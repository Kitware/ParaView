import pydoc, re

class ParsePyDocOutput:
    def __init__(self, package, module, function):
        self.desc = ''
        self.data_mems = ''
        self.inh_data_mems = {}
        self.method_mems = ''
        self.inh_method_mems = {}
        self.proxy_modules = ['filters', 'sources', 'writers', 'animation']
        self._parse(package, module, function)


    def _parse(self, package, module, function):
        """
        Get pydoc output on the module generated and parse that output to
        extract information about data members and methods

        package  - Package that the module belongs to (type: str)
        module   - Hierarchically sorted list of modules(type: list)
        function - Function/Class of interest (type: str)
        """
        pac = __import__('%s' % package)
        mod = pac
        for modul in module:
            mod = getattr(mod, '%s' % modul)
        func = None
        for prx_mod in self.proxy_modules:
            try:
                proxy_mod = getattr(mod, '%s' % prx_mod)
                func = getattr(proxy_mod, '%s' % function)
            except AttributeError:
                continue
            break
        if not func:
            raise Exception('Proxy not found: %s %s' % (function, modul))
        doc_to_parse = pydoc.render_doc(func)
        doc_to_parse = re.sub(r'\x08.', '', doc_to_parse)
        # Append a block completion line at the very end for easier regex matches
        doc_to_parse = doc_to_parse + " |"\
        "  ----------------------------------------------------------------------\n"

        desc = re.search(r'(?<=SourceProxy\)).+?(?=Method resolution order)',
                doc_to_parse, re.DOTALL)
        if desc:
            desc = desc.group()
            common_desc = re.search(r'(Proxy for a server).+?(vtkSMProxy C\+\+ class.)',
                    desc, re.DOTALL)
            if common_desc:
                desc = re.sub(common_desc.re, '', desc)
            self.desc = desc.replace('|','').strip()

        data_mems = re.search(r'(?<=Data descriptors defined here:).+?(?=---------)',
                doc_to_parse, re.DOTALL)
        if data_mems:
            self.data_mems = data_mems.group().replace('|','')

        inh_data_mems = re.findall(r'(?<=Data descriptors inherited from).+?(?=-------)',
                doc_to_parse, re.DOTALL)
        for mem in inh_data_mems:
            inh_data_mem_class = re.search(r'.+?(?=:)(:)', mem)
            self.inh_data_mems[inh_data_mem_class.group().replace(':','').strip()] =\
                re.sub(inh_data_mem_class.re, '', mem).replace('|','')

        method_mems = re.search(r'(?<=Methods defined here:).+?(?=--------)',
                doc_to_parse, re.DOTALL)
        if method_mems:
            self.method_mems = method_mems.group().replace('|', '')

        inh_method_mems = re.findall(r'(?<=Methods inherited from).+?(?=--------)',
                doc_to_parse, re.DOTALL)
        for mem in inh_method_mems:
            inh_method_mem_class = re.search(r'.+?(?=:)(:)', mem)
            self.inh_method_mems[inh_method_mem_class.group().replace(':','').strip()] =\
                re.sub(inh_method_mem_class.re, '', mem).replace('|','')
