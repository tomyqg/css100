#include <Poco/Timestamp.h>
#include <Poco/Thread.h>
#include <Poco/StringTokenizer.h>
#include <Poco/NumberParser.h>
#include <Poco/String.h>
#include <glog/logging.h>
#include <yatengine.h>
#include <Poco/SingletonHolder.h>
#include <ctype.h>
#include <stdio.h>
#include "bythread.h"
#include "termcommu.h"
#include "udpdtu.h"
#define CTX_DBG
using namespace TelEngine;


static Poco::Timestamp lastDateTime,sendInfoTime;
static Poco::FastMutex _mutex;

TermCommu::TermCommu()
{
    dtu_dev = NULL;
}
bool TermCommu::start(long timeout_ms)
{
    dtu_dev = new UdpDtu();
    if(NULL==dtu_dev) return false;
    if(false == dtu_dev->start (timeout_ms)) return false;

    //ByThread::start (timeout_ms);
}
TermCommu& TermCommu::GetInstance()
{
    static Poco::SingletonHolder<TermCommu> sh;
    return *sh.get ();
}

size_t  TermCommu::CheckMessage(int type)
{
    Poco::FastMutex::ScopedLock lock(_mutex);

    if(type == 1)
        return m_msg_que.size();
    else if(type == 2)
        return m_height_msg_que.size();
    else if(type == 3)
        return m_ws_msg_que.size();
    return 0;
}
int  TermCommu::GetMessage(CTX_Message& msg,int type, int timeout_ms)
{
    //fprintf(stderr,"msg num = %d\n",m_recv_worker->m_msg_que.size());
    Poco::FastMutex::ScopedLock lock(_mutex);

    if(type == 1)
    {
        if(m_msg_que.size() > 0 )
        {
            msg = m_msg_que.front();
            m_msg_que.pop();
            return 1;
        }else
        {
            return 0;
        }

    }else if(type == 2)
    {
        if(m_height_msg_que.size() > 0 )
        {
            msg = m_height_msg_que.front();
            m_height_msg_que.pop();
            return 1;
        }else
           return 0;
    }else if(type == 3)
    {
        if(m_ws_msg_que.size() > 0 )
        {
            msg = m_ws_msg_que.front();
            m_ws_msg_que.pop();
            return 1;
        }else
           return 0;
    }
    return 0;


}
int  TermCommu::SendMessage(CTX_Message& msg)
{
    return SendMessage(msg.context);
}
int  TermCommu::SendMessage(std::string& msg)
{
    if(dtu_dev)
    {
        return dtu_dev->SendBuffer ((unsigned char*)msg.c_str (),msg.length ());
    }
    return 0;
}
int  TermCommu::GetReceivedCount()
{
    return 0;
}
void TermCommu::ClearMessage(void)
{

}
void TermCommu::ResetCounter(void)
{

}
/*********************************************************************************************
 *�������в���һ�����ݰ������յ������ݰ�����ȡ���͸����ݰ����豸ID�������е�����ID�������������ID
 *
***********************************************************************************************/
bool      TermCommu::DripDeviceNo(int &SenderNo, int &RightNo, int &AddNo)
{
    CTX_Message msg;
    if(GetMessage(msg))
    {
          int    right_id,addin_id,sender_id;
          double angle,position,AngleSpeed,Dang;
          DLOG(INFO) << "DripDeviceNo" << msg.context;
          //CTX_DBG("DripMainNoAndAddNo %s\n",msg.context.c_str());
          Poco::StringTokenizer token(msg.context,"N");

#ifdef CTX_DEBUG
            for(size_t i = 0 ; i < token.count(); i++)
            {
                CTX_DBG("token[%d]=%s\n",i,token[i].c_str());
            }
#endif
          if(token.count() != 7)
          {
               CTX_DBG("DripMainNoAndAddNo Error MsgCount =%d\n",token.count());
               return false;
          }

          if(!Poco::NumberParser::tryParse(Poco::trim(token[0]),sender_id))return false;
          if(!Poco::NumberParser::tryParse(Poco::trim(token[1]),right_id))return false;
          if(!Poco::NumberParser::tryParseFloat(Poco::trim(token[2]),angle))return false;
          if(!Poco::NumberParser::tryParseFloat(Poco::trim(token[3]),position))return false;
          if(!Poco::NumberParser::tryParseFloat(Poco::trim(token[4]),AngleSpeed))return false;
          if(!Poco::NumberParser::tryParseFloat(Poco::trim(token[5]),Dang))return false;
          if(!Poco::NumberParser::tryParse(Poco::trim(token[6]),addin_id))return false;

          if(sender_id > 0 && sender_id <= 20 )
          {
              SenderNo  = sender_id;
              RightNo   = right_id;
              AddNo     = addin_id;
              return true;
          }
          return false;
    }
}
/*
��������,����������磬��ȡ��������״̬.

*/
void      TermCommu::WatchNetWork(int local_id,int &master_id, bool &AddState)
{
    Poco::Timestamp StartTime;
    Poco::Timestamp CurTime;
    CTX_Message msg;
    int right_id  = 0;
    int addin_id  = 0;
    int sender_id = 0;
    bool FoundM = false;


    ClearMessage(); //���������֮ǰҪ�Ȱѻ��������ȫ������
    //listen 5s to recv rtmsg;
    ResetCounter();
    while( (CurTime-StartTime) < 10000000) //�ȴ�10s���Ƿ����յ�������ȡ�����ӻ�������
    {
        if(GetMessage(msg))//��������п������������ģ�Ҳ�п����Ǵӻ���Ӧ��
        {
                DLOG(INFO) << "WatchNetWork Capture Message ----> Maybe Find Master\n";
                FoundM = true;
                break;
        }

        Poco::Thread::sleep(50);

        CurTime.update();
    }

    if(FoundM)
    {
        //�ȴ�100ms���ٽ���һ����Ϣ,����100ms�п����Ѿ��յ��˺ü�����
        Poco::Thread::sleep(100);
        if(CheckMessage())
        {
            DripDeviceNo(sender_id,right_id,addin_id);
        }
        /*
            right_id == 0: ����������Ǵӻ��Ļ�Ӧ����
            addin_id != local_id : ���������������Ҫ�ͱ����ı��һ�£�Ҳ���Ǳ������յ����뱾����������ݰ�
            master_id == local_id: ����id�����뱾��idһ�£���Ϊ��Ȼ�յ����źţ��϶������Ǵӻ���
        */
        while( (right_id == 0) || (addin_id != local_id) || (sender_id == local_id) ){
            Poco::Thread::sleep(10);
            if(CheckMessage())
            {
                DripDeviceNo(sender_id,right_id,addin_id);
                //DLOG(INFO��<< "DripMainNoAndAddNo Ok MainId" << sender_id << "RightId=" << right_id << "AddNo=" << addin_id;
            }
        }
        master_id = sender_id;
        CTX_DBG("WatchNetWork OK MainId=%d RightId=%d AddNo=%d\n",master_id,right_id,addin_id);
       // g_TC[m_local_id].Valide = true;
        //g_TC[m_main_id].Valide  = true;
        AddState = true;

        StartTime.update();
        CurTime=StartTime;
        CTX_DBG("[Ready Add TC] Wait Slave Ack\n");
        //�ٽ���һ����Ч����Ȼ���ͱ�����Ϣ [���ǽ��ձ���ѯ�Ĵӻ�����������Ϣ������Ҫ���յ������Ϣ�󣬲��ܷ��ͣ��������ͬʱ���͵ĳ�ͻ]
        //��Ϊ�¼����������Ǹ�����������ѯ��Ϣ�еģ����Ա���ѯ����Ҳ���Ӧ��Ϣ��������������Ӧ�����¼���������ȴ�20ms�󣬲����յ��˸û�Ӧ
        //��Ż��Ӧ�Լ�����Ϣ,���ֻ��һ������������£���ô��������һ������ָ��1s����Ҳ���յ����ʿ��Լ���ɹ�
        while (!GetMessage(msg))
        {
            CurTime.update();
            if( (CurTime-StartTime) > 1200000)
            {
                //1200ms �ȴ�1200ms�Ա���ղ�ѯ�����Ļ�Ӧ��Ϣ
                AddState=false;
            }
            Poco::Thread::sleep(20);
            CTX_DBG("New Add TC wait [Slave RightNo Ack]\n");
        }
        if(!AddState)
        {
            CTX_DBG("[Ready Add TC] Wait Slave Ack Failed\n");
            return;
        }
        //����ΪʲôҪ��Ӧ����,��ΪҪ����������������
        std::string sendInfo = "";//build_qurey_msg();
        SendMessage(sendInfo);
        lastDateTime.update();
        sendInfoTime=lastDateTime;

    }
    else
    {
        //�ϸ�Ҫ�������ڵȴ������в����յ��κ�һ������,���ܱ�Ϊ����
        if( GetReceivedCount() > 0) //��Ȼû���յ���ȷ�����ݣ������п������յ������롣����Ҳ���������������
        {
            CTX_DBG("WatchNetwork Failed beacuse of GetReceivedCount=%d\n",GetReceivedCount());
            AddState  = false;
        }
        else
        {
            AddState  = true;
        }

    }

}


/*�������յ�������*/
void TermCommu::ParseRecvData(char c)
{
    static int work_site_count=0;
    int type = 0;

    if( (c == '%') || (c=='(') || (c=='$'))
    {
        m_pos = 0;
        m_start_flag = true;
    }
    else if( m_start_flag && c=='#')
    {
        if(m_pos == 38)
        {
            type = 1;

        }
    }
    else if( m_start_flag && c=='*')
    {
        if(m_pos == 11)
        {
            type = 2;
        }
    }
    else if( m_start_flag && c==')')
    {
        type = 3;
    }
    if(c=='*')
    {
        work_site_count++;
        if(work_site_count >= 3)
        {
            m_allow_send=false;
            m_upload_stamp.update ();
        }
    }
    else
    {
        work_site_count = 0;
    }
    if( ( m_start_flag ) && (m_pos < MAX_LEN) )
        m_tmp[m_pos++]=c;

    if(type > 0){
        if((m_pos>=1) && (m_pos < MAX_LEN))
        {
            m_tmp[m_pos-1] = 0;

            CTX_Message msg;
            msg.context = std::string(m_tmp+1);
            msg.wType   = type;
            if(type == 1)
            {
                Poco::FastMutex::ScopedLock lock(_mutex);
                m_signal = 5;
                m_receive_msg_count++;
                if(m_msg_que.size () < 1000)
                    m_msg_que.push(msg);
                if(m_msg_que.size() > 10)
                    fprintf(stderr,"m_msg_que push data size=%d\n",m_msg_que.size());

            }
            else if(type == 2)
            {
                Poco::FastMutex::ScopedLock lock(_mutex);
                 if(m_height_msg_que.size () < 10)
                    m_height_msg_que.push(msg);

                fprintf(stderr,"recv height msg %s\n",msg.context.c_str());
                if(m_height_msg_que.size() > 10)
                    fprintf(stderr,"m_height_msg_que push data size=%d\n",m_height_msg_que.size());
            }
            else if(type == 3) // worksite
            {
                Poco::FastMutex::ScopedLock lock(_mutex);
                if(m_ws_msg_que.size () < 10)
                    m_ws_msg_que.push(msg);
                m_allow_send=true;
                if(m_ws_msg_que.size() > 10)
                    fprintf(stderr,"m_ws_msg_que push data size=%d\n",m_ws_msg_que.size());
            }
            m_start_flag = false;

        }

    }


}

bool checkValid(char c)
{
    return isprint (c);
}

void TermCommu::ParseData(void)
{

}
void TermCommu::OnDataRecived(unsigned char *buffer,size_t len)
{

    if(len > 0)
    {
        for(size_t i = 0; i < len; i++)
        {
            char c = buffer[i];
            if(checkValid(c)){
               ParseRecvData(c);
            }else{
                fprintf(stderr,"***********************Invalid char 0x%x\n",c);
            }
        }
    }
}