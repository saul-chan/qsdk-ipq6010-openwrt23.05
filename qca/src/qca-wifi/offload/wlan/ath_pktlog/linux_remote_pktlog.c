/*
 * Copyright (c) 2014-2015,2017 Qualcomm Innovation Center, Inc.
 *
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Innovation Center, Inc.
 *
 *
 * 2014-2015 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

#if REMOTE_PKTLOG_SUPPORT

#include <linux_remote_pktlog.h>
#include <pktlog_ac.h>
#include <pktlog_ac_fmt.h>
#include <pktlog_ac_api.h>

/*
 * Function     : stop_service
 * Description  : stop remote packet log service.
 *                release allocated resources like sockets etc.
 * Input        : pointer to scn
 * Return       : 0 on success, negative error code on failure.
 */
int stop_service(struct ol_ath_softc_net80211 * scn)
{
    ol_pktlog_dev_t *pl_dev;
    struct pktlog_remote_service *service;
    pl_dev  = scn->pl_dev;
    service = &pl_dev->rpktlog_svc;

    /* free allocated resources before exit */
    if (service->send_socket != NULL) {
        sock_release(service->send_socket);
        service->send_socket = NULL;
    }

    if (service->listen_socket != NULL) {
        sock_release(service->listen_socket);
        service->listen_socket = NULL;
    }

    qdf_nofl_info("Missed Count: %d Fends: %d \n", service->missed_records, service->fend_counts);
    return 0;
}

/*
 * Function     : pktlog_remote_service_send
 * Description  : This function sends packetlog buffer to sockets.
 * Input        : pointer to packetlog remote service , buffer and length.
 * Return       : size of data that sent through socket.
 */
int pktlog_remote_service_send(struct pktlog_remote_service *service, char *buf, int len)
{
    struct msghdr msg = {0};
    struct iovec iov;
    int size = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,11,0)
    mm_segment_t oldfs;
#endif

    if (!service->connect_done) {
        return 0;
    }

    iov.iov_base = buf;
    iov.iov_len = len;

    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,6,0)
    msg.msg_iter.__iov = &iov;
    msg.msg_iter.count = len;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0)
    msg.msg_iter.iov = &iov;
    msg.msg_iter.count = len;
#else
    msg.msg_iov = &iov;
    msg.msg_iovlen = len;
#endif
    msg.msg_name = 0;
    msg.msg_namelen = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,11,0)
    oldfs = get_fs();
    set_fs(KERNEL_DS);
#endif
    if (service->send_socket) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,1,0)
        size = sock_sendmsg(service->send_socket, &msg);
#else
        size = sock_sendmsg(service->send_socket, &msg, len);
#endif
    }
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,11,0)
    set_fs(oldfs);
#endif

    return size;
}

/*
 * Function     : pktlog_run_send_service - WorkQueue
 * Description  : This is main core function that manipulates packetlog read,
                  write buffers and take decession on how much data we need
                  to send via sockets.
 * Input        : pointer to context (scn).
 * Return       : None
 */
void pktlog_run_send_service(void *context)
{
    struct ol_ath_softc_net80211 * scn = (struct ol_ath_softc_net80211 *) context;
    ol_pktlog_dev_t *pl_dev;
    struct ath_pktlog_info *pl_info;
    struct ath_pktlog_buf *log_buf;
    struct pktlog_remote_service *service;
    char *buffer;
    DECLARE_WAIT_QUEUE_HEAD(wq);

    pl_dev  = scn->pl_dev;
    pl_info = pl_dev->pl_info;
    log_buf = pl_info->buf;
    service = &pl_dev->rpktlog_svc;

    if (!service || !service->send_socket)
        return;

    buffer = (char *) &(pl_info->dev_unique_id);
    if (pktlog_remote_service_send(service, buffer, PKTLOG_UNIQUE_ID_SIZE) <= 0)
        goto release_sock;

    buffer = (char *) &(pl_info->radio_name);
    if (pktlog_remote_service_send(service, buffer, RADIO_NAME_SIZE) <= 0)
        goto release_sock;

    buffer = (char *) &(log_buf->bufhdr);
    if (pktlog_remote_service_send(service, buffer, PKTLOG_HEADER_SIZE) <= 0)
        goto release_sock;

    wait_event_timeout(wq, 0, WAIT_EVENT_TIMEOUT_MAX);

    while (TRUE) {

        buffer = &(log_buf->log_data[pl_info->rlog_read_index]);

        if(!pl_info->is_wrap) {
            /* send min of MAX_SEND_SIZE or pending packets */
            if((pl_info->rlog_write_index - pl_info->rlog_read_index) >= MAX_SEND_SIZE) {
                if (pktlog_remote_service_send(service, buffer, MAX_SEND_SIZE) <= 0)
                    break;
                pl_info->rlog_read_index += MAX_SEND_SIZE;
            } else {
                if (service->running) {
                    wait_event_timeout(wq, 0, WAIT_EVENT_TIMEOUT_MIN);
                    continue;
                } else {
                    if (pktlog_remote_service_send(service, buffer, (pl_info->rlog_write_index - pl_info->rlog_read_index)) <= 0)
                        break;
                    pl_info->rlog_read_index += (pl_info->rlog_write_index - pl_info->rlog_read_index);
                    qdf_nofl_info("Service Stopped: sending last %d bytes \n", (pl_info->rlog_write_index - pl_info->rlog_read_index));
                }
            }
        } else {
            /* Write index is wrapped */
            if((pl_info->rlog_max_size - pl_info->rlog_read_index >= MAX_SEND_SIZE)) {
                if (pktlog_remote_service_send(service, buffer, MAX_SEND_SIZE) <= 0)
                    break;
                pl_info->rlog_read_index += MAX_SEND_SIZE;
            } else { /* less than MAX_SEND_SIZE */
                if (pl_info->rlog_max_size - pl_info->rlog_read_index) {
                    if (pktlog_remote_service_send(service, buffer, (pl_info->rlog_max_size - pl_info->rlog_read_index)) <= 0)
                        break;
                    pl_info->rlog_read_index += (pl_info->rlog_max_size - pl_info->rlog_read_index);
                }
            }

            if (pl_info->rlog_max_size == pl_info->rlog_read_index) {
                service->fend_counts++;
                pl_info->rlog_read_index = 0;
                pl_info->rlog_max_size = pl_info->buf_size;
                pl_info->is_wrap = 0;
            }
        }

        /* Break if read == write and service is stopped */
        if (!service->running && (pl_info->rlog_write_index == pl_info->rlog_read_index)) {
            break;
        }
        if(signal_pending(current))
            break;
    }

release_sock:
    stop_service(scn);
    return;
}

/*
 * Function     : pktlog_start_accept_service - WorkQueue
 * Description  : This function accepts incomming connections
                  to remote packetlog socket and schedule send
                  service.
 * Input        : pointer to context (scn).
 * Return       : None
 */
void pktlog_start_accept_service(void *context)
{
    struct socket *socket = NULL;
    int error = 0;
    struct ol_ath_softc_net80211 * scn = (struct ol_ath_softc_net80211 *) context;
    ol_pktlog_dev_t *pl_dev = NULL;
    struct pktlog_remote_service *service = NULL;
    struct inet_connection_sock *isock = NULL;
    DECLARE_WAITQUEUE(wait, current);

    pl_dev = scn->pl_dev;
    service = &pl_dev->rpktlog_svc;

    if (!service)
        return;

    socket = service->listen_socket;
    if (socket == NULL)
        return;

    service->send_socket = sock_alloc();
    if (service->send_socket == NULL)
        return;

    service->send_socket->type = socket->type;
    service->send_socket->ops = socket->ops;

    /*check the accept queue*/
    isock = inet_csk(socket->sk);
    while (service->running) {
        if((reqsk_queue_empty(&isock->icsk_accept_queue))) {
            if (!socket) {
                add_wait_queue(&socket->sk->sk_wq->wait, &wait);
            }
            __set_current_state(TASK_INTERRUPTIBLE);
            schedule_timeout(HZ);
            __set_current_state(TASK_RUNNING);
            if (!socket) {
                remove_wait_queue(&socket->sk->sk_wq->wait, &wait);
            }
            continue;
        }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
        error = socket->ops->accept(socket, service->send_socket, O_NONBLOCK, true);
#else
        error = socket->ops->accept(socket, service->send_socket, O_NONBLOCK);
#endif
        if(error < 0) {
            qdf_nofl_info("accept error,release the socket\n");
            sock_release(service->send_socket);
            service->send_socket = NULL;
            return;
        }

        service->connect_done = TRUE;
        qdf_sched_work(scn->soc->qdf_dev, &service->send_service);
        if (service->connect_done) {
            break;
        }
    }

    if (!service->connect_done)
        stop_service(scn);

    return;
}

/*
 * Function     : pktlog_start_remote_service - WorkQueue
 * Description  : This function creates remote packetlog socket
                  and schedules accept service.
 * Input        : pointer to context (scn).
 * Return       : None
 */
void pktlog_start_remote_service(void *context)
{
    int error = 0;
    struct socket *socket = NULL;
    struct sockaddr_in bind_address;
    struct ol_ath_softc_net80211 * scn = (struct ol_ath_softc_net80211 *) context;
    ol_pktlog_dev_t *pl_dev = NULL;
    struct pktlog_remote_service *service = NULL;
    int opt = 1;
    DECLARE_WAIT_QUEUE_HEAD(wq);

    pl_dev = scn->pl_dev;
    service = &pl_dev->rpktlog_svc;

    if (!service)
        return;

    service->running = TRUE;
    error = sock_create(PF_INET, SOCK_STREAM,IPPROTO_TCP, &service->listen_socket);
    if(error < 0){
        qdf_nofl_info(KERN_ERR "%s: listen socket create error \n", __func__);
        return;
    }

    socket = service->listen_socket;
    socket->sk->sk_reuse = 1;

    memset(&bind_address, 0, sizeof(bind_address));
    bind_address.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_address.sin_family = AF_INET;
    bind_address.sin_port = htons(service->port);

    error = socket->ops->setsockopt(socket, SOL_TCP, TCP_NODELAY, (void *)&opt, sizeof(opt));
    if (error < 0)
        qdf_nofl_info(KERN_ERR "%s: send socket create error \n", __func__);

    error = socket->ops->bind(socket, (struct sockaddr*)&bind_address, sizeof(bind_address));
    if(error < 0) {
        qdf_nofl_info(KERN_ERR "%s: Error in bind address \n", __func__);
        return;
    }

    error = socket->ops->listen(socket, MAX_LISTEN_CONECTIONS);
    if(error < 0) {
        qdf_nofl_info(KERN_ERR "%s: Error in listen \n", __func__);
        return;
    }

    qdf_sched_work(scn->soc->qdf_dev, &service->accept_service);
    return;
}

void pktlog_start_remote_service_client(void *context)
{
    int error = 0;
    struct socket *socket = NULL;
    struct sockaddr_in serv_address;
    struct ol_ath_softc_net80211 * scn =
                          (struct ol_ath_softc_net80211 *) context;
    ol_pktlog_dev_t *pl_dev = NULL;
    struct ath_pktlog_info *pl_info = NULL;
    struct pktlog_remote_service *service = NULL;
    DECLARE_WAIT_QUEUE_HEAD(wq);

    pl_dev = scn->pl_dev;
    pl_info = pl_dev->pl_info;
    service = &pl_dev->rpktlog_svc;

    if (!service)
        return;

    service->running = TRUE;
    error = sock_create(PF_INET,
                        SOCK_STREAM,IPPROTO_TCP, &service->send_socket);
    if(error < 0){
        qdf_nofl_info(KERN_ERR "%s: send socket create error \n", __func__);
        return;
    }

    socket = service->send_socket;
    socket->sk->sk_reuse = 1;

    qdf_nofl_info("IP4 address to connect : %s\n", pl_info->ipaddr);
    qdf_nofl_info("ClientPortNumber =%d\n",service->port);
    qdf_mem_zero(&serv_address, sizeof(serv_address));
    serv_address.sin_addr.s_addr = in_aton(pl_info->ipaddr);

    serv_address.sin_family = AF_INET;
    serv_address.sin_port = htons(service->port);
    error = socket->ops->connect(socket, (struct sockaddr*) &serv_address,
                                 sizeof(serv_address), 0);
    if(error < 0) {
        qdf_nofl_info(KERN_ERR "%s: Error in connect \n", __func__);
        return;
    }

    qdf_nofl_info("Connection Successful with server\n");
    service->connect_done = TRUE;
    qdf_sched_work(scn->soc->qdf_dev, &service->send_service);
    return;
}

/*
 * Function     : pktlog_remote_enable
 * Description  : This function initializes remote packetlog
                  service structures and schedules connection service.
 * Input        : pointer to context (scn).
 * Return       : 0 on success, negative error code on failure.
 */
int pktlog_remote_enable(struct ol_ath_softc_net80211 *scn, int enable)
{
    ol_pktlog_dev_t *pl_dev = NULL;
    struct ath_pktlog_info *pl_info = NULL;
    struct pktlog_remote_service *service = NULL;

    pl_dev = scn->pl_dev;
    pl_info  = pl_dev->pl_info;
    service = &pl_dev->rpktlog_svc;

    if (!service)
        return 0;

    if ((enable && !service->running) &&
        (service->send_socket == NULL)) {
        /* service initialization */
        service->listen_socket = NULL;
        service->send_socket = NULL;
        service->connect_done = 0;
        service->running = TRUE;
        service->missed_records = 0;
        service->fend_counts = 0;
        /* configured by applcation */
        if (pl_info->remote_port)
            service->port = pl_info->remote_port;

        qdf_nofl_info("%s , port %d \n", __func__, service->port);
        /* init buffer related counters/index */
        pl_info->rlog_max_size = pl_info->buf_size;
        pl_info->rlog_write_index = 0;
        pl_info->rlog_read_index = 0;
        pl_info->is_wrap = 0;
        if (pl_info->pktlog_remote_client) {
            service->port = pl_info->remote_port;
            qdf_sched_work(scn->soc->qdf_dev, &service->client_service);
        } else {
            qdf_sched_work(scn->soc->qdf_dev, &service->connection_service);
        }
    } else if (!enable) {
        service->running = 0;
        pl_info->pktlog_remote_client = FALSE;
        qdf_mem_zero(&pl_info->ipaddr[0], IP_ADDR_STR_MAX);
        qdf_nofl_info("Stopping Service: write index: %d read index: %d \n", pl_info->rlog_write_index, pl_info->rlog_read_index);
    }

    return 0;
}

#endif

