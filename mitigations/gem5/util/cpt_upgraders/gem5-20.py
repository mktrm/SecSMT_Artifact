# Update

def rename_section(cp, section_from, section_to):
    items = cp.items(section_from)
    cp.add_section(section_to)
    for item in items:
        cp.set(section_to, item[0], item[1])
    cp.remove_section(section_from)

def upgrader(cpt):
    for sec in cpt.sections():
        if cpt.has_option(sec, "currPwrState"):
            cpt.remove_section(sec)
        import re
        if re.search('system.*cpu.xc.*', sec):
            cpt.set(sec, "vecPredRegs", "0000000")
        if re.search("system.cpu.workload.Entry.*", sec):
            newsec = sec.replace("system.cpu.workload.Entry",
                'system.cpu.workload.ptable.Entry')
            cpt.add_section(newsec)
            items = cpt.items(sec)
            for item in items:
                cpt.set(newsec, item[0], item[1])
            cpt.remove_section(sec)
        if cpt.has_option(sec, "ptable.size"):
            newsec = "system.cpu.workload.ptable"
            cpt.add_section(newsec)
            cpt.set(newsec, "size", cpt.get(sec, "ptable.size"))
        if re.search('system.cpu..tb',sec):
            cpt.remove_section(sec)
    newsec = "system.cpu.workload.vmalist"
    cpt.add_section(newsec)
    cpt.set(newsec, "size", "2")

    newsec = "system.cpu.workload.vmalist.Vma0"
    cpt.add_section(newsec)
    cpt.set(newsec, "name", "stack")
    cpt.set(newsec, "addrRangeStart", "140737488347136")
    cpt.set(newsec, "addrRangeEnd", "140737488351232")


    newsec = "system.cpu.workload.vmalist.Vma1"
    cpt.add_section(newsec)
    cpt.set(newsec, "name", "vsyscall")
    cpt.set(newsec, "addrRangeStart", "18446744073699065856")
    cpt.set(newsec, "addrRangeEnd", "18446744073699069952")


legacy_version = 7
