import os
import shutil
import sys

def removebb(s):
    if (len(s) == 2): return '0'
    else: return s[2:]

def extract_and_format_digits(s):
    a = s.split('.')
    if (len(a) == 1): return removebb(s)
    bb1 = removebb(a[0])
    a = '.'.join(a[1:]) 
    if (a == 'loopexit'): return bb1+'.le'
    pos = a.find('_')
    bb2 = removebb(a[0:pos])
    if pos == -1: bb2 = removebb(a)
    args = a[pos+1:]
    if (args == 'crit_edge'):
        return bb1+'_'+bb2+'.ce'
    else:
        return bb1+'_'+bb2+'.'+args

profile_dir = sys.argv[1]
functions = []

for i in os.listdir(profile_dir+'/dot'):
    if i[0] == '.' and i[-4:] == '.dot':
        functions.append(i[1:-4])

for f in functions:
    try:
        dot_file = open(profile_dir+'/dot/.'+f+'.dot','r')
        new_dot_file = open('/tmp/'+f+'.dot','w')
        bb_prof_file = open(profile_dir+'/profiles/'+f+'.prof.full.bb','r')
        edges_prof_file = open(profile_dir+'/profiles/'+f+'.prof.full.edges','r')
    except:
        continue

    dot_data = dot_file.readlines()
    bb_prof_data = bb_prof_file.readlines()
    edges_prof_data = edges_prof_file.readlines()

    dot_file.close()
    bb_prof_file.close()
    edges_prof_file.close()

    bb_profile = {}
    edges_profile = {}

    for i in bb_prof_data:
        if i.find(':') == -1: continue
        aux = i.replace('\n','').split(' : ')
        bb_profile[aux[0]] = aux[1]
        edges_profile[aux[0]] = {}

    for i in edges_prof_data:
        if i.find(':') == -1: continue
        aux = i.replace('\n','').split(' : ')
        bbs = aux[0].split(' -> ')
        edges_profile[bbs[0]][bbs[1]] = aux[1]

    try:
        for i in dot_data:
            if i.find('->') != -1:
                edge_start_pos = i.find('tooltip')+9
                edge_end_pos = i[edge_start_pos:].find('\\n')
                bbs = i[edge_start_pos:edge_start_pos+edge_end_pos].split(' -> ')
                bb1 = extract_and_format_digits(bbs[0])
                bb2 = extract_and_format_digits(bbs[1])
                print(i[:edge_start_pos-9]+'fontname="Courier", label="'+edges_profile[bb1][bb2]+'", '+i[edge_start_pos-9:],end='',file=new_dot_file)
            elif i.find('Node0x') != -1:
                bb_start_pos = i.find('label')+8
                bb_end_pos = i[bb_start_pos:].find(':')
                bb_name = i[bb_start_pos:bb_start_pos+bb_end_pos]
                bb_name = extract_and_format_digits(bb_name)
                print(i[:bb_start_pos]+'bb'+bb_name+': '+bb_profile[bb_name]+i[bb_start_pos+bb_end_pos+1:],end='',file=new_dot_file)
            else:
                print(i,end='',file=new_dot_file)
        
        new_dot_file.close()

        shutil.copyfile('/tmp/'+f+'.dot', profile_dir+'/dot/'+f+'.dot')
    except:
        print('Error adding profile information to dot file for function ' + f)
        print('Aborting')