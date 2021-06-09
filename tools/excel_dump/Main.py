# -*- coding: utf-8 -*-
 
'''
Created on 20160412

@author: hasee
'''
import json
import os
import logging
import traceback

from xlrd import open_workbook  
from _symtable import CELL
from telnetlib import theNULL

  
logger = logging.getLogger('mylogger')  
logger.setLevel(logging.DEBUG)  
# 创建一个handler，用于写入日志文件    
fh = logging.FileHandler('DumpInfo.log', mode='w')  
  
# 再创建一个handler，用于输出到控制台    
ch = logging.StreamHandler() 

# 定义handler的输出格式formatter    
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')  
fh.setFormatter(formatter)  
ch.setFormatter(formatter)  

logger.addHandler(fh)  
logger.addHandler(ch) 

def dumpExcel(sNmae):
    wb = open_workbook(sNmae)

    for s in wb.sheets():
        # print "*" * 10
        # print 'Sheet:',s.name
        logger.info("*" * 10)  
        logger.info('File:%s,Sheet:%s'%(sNmae,s.name,))    
        
        # 列字段类型
        field_type = {}
        field_name = {}
        field_key = []
            
        lDocument = []
            
        try:
            for row in range(s.nrows):  
                dElem = {}
                for col in range(s.ncols):
                    #第一列为备注，忽略
                    if 0 == col:
                        continue

                    #字段类型
                    if row > 4:
                        iFieldLen = len(field_type)
                        if col > iFieldLen:
                            break
                    
                    cellVal = s.cell(row, col).value
                    strType = type(cellVal)
                    #print("type:%s", strType)

                    if isinstance(cellVal, str):
                        sCellVal = cellVal
                        #0:注释行1, 1:表头, 2:注释行2, 3:字段描述
                        if row in (0, 1, 2, 3):
                            continue
                        #4:字段类型, 有效的字段类型(key, int, float, string ,str), 第一个字段类型必须是key
                        elif 4 == row:
                            if sCellVal == "":
                                break
                            field_type[col] = sCellVal

                            if sCellVal in ("key",):
                                field_key.append(col)

                            if 1 == col:
                                if sCellVal not in ("key",):
                                    print("row:%s:col:%s|must be key:%s" % (row, col, sType))
                                    logger.error("ErrorInfo:row:%s:col:%s|must be key:%s" % (row, col, sType))
                                    raise Exception("InvalidType|row:%s:col:%s|must be key:%s" % (row, col, sType))

                        #5:server字段名
                        elif 5 == row:
                            field_name[col] = sCellVal

                            if sCellVal in ("List", ):
                                print("row:%s:col:%s|invalid type name:%s" % (row, col, sCellVal))
                                logger.error("ErrorInfo:row:%s:col:%s|invalid type name:%s" % (row, col, sCellVal))
                                raise Exception("InvalidType|row:%s:col:%s|invalid type name:%s" % (row, col, sCellVal))


                        #6:client字段名
                        elif 6 == row:
                            continue
                        else:
                            sType = field_type[col]
                            sFieldName = field_name[col]
                            if sType in ("int", "key"):
                                if sCellVal == "":
                                    sCellVal = "0"
                                iValue = int(sCellVal)
                                dElem[sFieldName] = iValue
                            elif sType == "float":
                                if sCellVal == "":
                                    sCellVal = "0"
                                fValue = float(sCellVal)
                                dElem[sFieldName] = fValue                        
                            elif sType in ("string", "str"):
                                sValue = sCellVal
                                dElem[sFieldName] = sValue
                            elif sType == "dict":
                                if sCellVal == "":
                                   dValue = {}
                                   dElem[sFieldName] = dValue
                                else:
                                    dValue = eval(sCellVal)
                                    dElem[sFieldName] = dValue
                            elif sType == "list":
                                if sCellVal == "":
                                    lValue = []
                                    dElem[sFieldName] = lValue
                                else:
                                    lValue = eval(sCellVal)
                                    dElem[sFieldName] = lValue
                            else:
                                print("row:%s:col:%s|unicode error type:%s"%(row, col, sType))
                                logger.error("ErrorInfo:row:%s:col:%s|unicode error type:%s"%(row, col, sType))
                                raise Exception("InvalidType|row:%s:col:%s|unicode error type:%s"%(row, col, sType))
                    elif isinstance(cellVal, float):
                        sCellVal = cellVal
                        # 0:注释行1, 1:表头, 2:注释行2, 3:字段描述
                        if row in (0, 1, 2, 3):
                            continue
                        # 4:字段类型, 有效的字段类型(key, int, float, string ,str)
                        elif 4 == row:
                            field_type[col] = sCellVal
                        # 5:server字段名
                        elif 5 == row:
                            field_name[col] = sCellVal
                        # 6:client字段名
                        elif 6 == row:
                            continue
                        else:
                            sType = field_type[col]
                            sFieldName = field_name[col]
                            if sType in ("int", "key"):
                                if sCellVal == "":
                                    sCellVal = "0"
                                iValue = int(sCellVal)
                                dElem[sFieldName] = iValue
                            elif sType == "float":
                                if sCellVal == "":
                                    sCellVal = "0"
                                fValue = float(sCellVal)
                                dElem[sFieldName] = fValue
                            elif sType in ("string", "str"):
                                sValue = str(int(sCellVal))
                                dElem[sFieldName] = sValue
                            elif sType == "dict":
                                if sCellVal == "":
                                   dValue = {}
                                   dElem[sFieldName] = dValue
                                else:
                                    dValue = eval(sCellVal)
                                    dElem[sFieldName] = dValue
                            elif sType == "list":
                                if sCellVal == "":
                                    lValue = []
                                    dElem[sFieldName] = lValue
                                else:
                                    lValue = eval(sCellVal)
                                    dElem[sFieldName] = lValue
                            else:
                                print("row:%s:col:%s|unicode error type:%s" % (row, col, sType))
                                logger.error("ErrorInfo:row:%s:col:%s|unicode error type:%s" % (row, col, sType))
                                raise Exception("InvalidType|row:%s:col:%s|unicode error type:%s" % (row, col, sType))
                    else:
                        print("row:%s:col:%s|other|"%(row, col), cellVal, type(cellVal))
                        raise Exception("InvalidCellType|row:%s:col:%s|other|" % (row, col), cellVal, type(cellVal))

                if 0 != len(dElem):
                    lDocument.append(dElem)            
        except:
            sErrorInfo = traceback.format_exc()
            logger.error("CatchException->ErrorPos:File:%s,Sheet:%s|row:%s:col:%s[%c]" % (sNmae, s.name, row+1, col+1, chr(65+col)))
            logger.error("ErrorInfo:%s" % (sErrorInfo, ))

            #print("CatchException->ErrorPos:File:%s,Sheet:%s|row:%s:col:%s[%c]" % (sNmae, s.name, row+1, col+1, chr(65+col)))

            input("导出出错,输入任意字符退出!")
            raise 

        lTransferDocument = []
        dLayer = {}

        for elems in lDocument:

            lCheck = []
            lCheck.append(dLayer)

            lLayerData = []
            lLayerData.append(lTransferDocument)

            for iCheck in range(0, len(field_key)):
                iKeyCol = field_key[iCheck]
                sKeyName = field_name[iKeyCol]
                iKeyValue = elems[sKeyName]

                dCheckAccess = lCheck[len(lCheck) - 1]

                if iKeyValue in dCheckAccess:
                    lCheck.append(dCheckAccess[iKeyValue])

                    lData = lLayerData[len(lLayerData) - 1]
                    if iCheck < len(field_key) - 1:
                        dListData = lData[len(lData)-1]
                        lLayerData.append(dListData["List"])
                    continue
                else:
                    iEnd = len(field_type) + 1
                    if iCheck + 1 < len(field_key):
                        iEnd = field_key[iCheck + 1]

                    dData = {}
                    for iCol in range(field_key[iCheck], iEnd):
                        sFieldName = field_name[iCol]
                        dData[sFieldName] = elems[sFieldName]
                    if iCheck < len(field_key) - 1:
                        dData["List"] = []

                    lData = lLayerData[len(lLayerData) - 1]
                    lData.append(dData)

                    if iCheck < len(field_key) - 1:
                        lLayerData.append(dData["List"])

                    dCheckAccess[iKeyValue] = {}
                    lCheck.append(dCheckAccess[iKeyValue])

        sJSON = json.dumps(lTransferDocument, ensure_ascii=False).encode('utf8')
        # print(sJSON)
        
        # Open a file
        sMiddleDir = "./DumpConfigs/"
        if not os.path.exists(sMiddleDir):
            os.makedirs(sMiddleDir)

        sFileName = sMiddleDir + s.name + ".json"
        fo = open(sFileName, "wb")
        fo.write(sJSON)
        
        # Close opend file
        fo.close()



def main():
    for _, _, files in os.walk('./'):
        for f in files:
            if f.endswith('.xls') or f.endswith('.xlsx'):
                try:
                    dumpExcel(f)
                except:
                    sErrorInfo = traceback.format_exc()
                    logger.error("dumpExcel error:File:%s," % (f))
                    logger.error("ErrorInfo:%s" % (sErrorInfo,))
                    input("导出出错,输入任意字符退出!")
    input("导出成功,输入任意字符退出!")

if __name__ == "__main__":
    main()
