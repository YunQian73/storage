#include "kvstore.h"

// 文件描述符
static int persist_fd = -1;

int kvstore_persist_init(const char *file_name){
    if(file_name == NULL){
        fprintf(stderr, "file_name is NULL!\n");
        return -1;
    }


    persist_fd = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if(persist_fd < 0){
        perror("open file failed\n");
        return -1;
    }

    return 0;
}

// 将buf中长度为len字节的数据写入persist_fd对应的文件中
int kvstore_persist_append(const char *buf, size_t len){
    if(buf == NULL || len == 0){
        fprintf(stderr, "buf or len can not be null\n");
        return -1;
    }

    ssize_t write_len = 0;
    while(write_len < len){
        ssize_t n = write(persist_fd, buf + write_len, len - write_len);
        if(n < 0){
            // 系统调用被信号中断
            if(errno == EINTR){
                continue;
            }

            perror("write file failed\n");

            return -1;
        }
        else if(n == 0){
            fprintf(stderr, "写入数据长度为0，导致死循环\n");
            return -1;
        }
        write_len += n;
    }
    return 0;
}

/*
数据同步函数，保证数据能够安全的存储到磁盘中
*/
int kvstore_persist_sync(void){
    if(persist_fd < 0){
        fprintf(stderr, "open file failed\n");
        return -1;
    }

    int n = fsync(persist_fd);
    if(n < 0){
        perror("fsync");
        return -1;
    }

    return 0;
}

void kvstore_persist_destroy(void){
    if(persist_fd < 0){
        fprintf(stderr, "fd < 0\n");
        return;
    }

    close(persist_fd);

    persist_fd = -1;
    return;
}


#if 0
void kvstore_aof_append(const char *file_name, const char *content){
    if(file_name == NULL){
        fprintf(stderr, "warning：文件名为空\n");
        return;
    }

    int file_fd = open(file_name, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if(file_fd == -1){
        perror("kvstore_aof_append : open file failed");
        return;
    }
    
    size_t file_len = strlen(content) + 3; // 预留"\r\n"与"\0"的位置
    char *final_content = (char*)malloc(file_len);
    if(final_content == NULL){
        fprintf(stderr, "malloc final_content failed\n");
        close(file_fd);
        return;
    }

    // 拼接字符串
    snprintf(final_content, file_len, "%s\r\n", content);

    ssize_t write_len = write(file_fd, final_content, file_len - 1); // 写入的内容不包括"\0"
    if(write_len == -1){
        close(file_fd);
        perror("写入文件失败\n");
    }
    else if(write_len != (file_len - 1)){
        fprintf(stderr, "写入的数据有失\n");
    }
    else{
        printf("成功写入文件\n");
    }

    free(final_content);
    close(file_fd);
    
    return;
}

// 从文件中读取数据
KvRecoverResult_t* kvstore_recover_data(const char* file){
    // 分配用于存放结果的内存
    KvRecoverResult_t* result = (KvRecoverResult_t*)malloc(sizeof(KvRecoverResult_t));
    if(result == NULL){
        perror("malloc kvrecoverresult failed");
        return NULL;
    }
    result->cmd_count = 0;
    result->cmds = NULL;

    int file_fd = open(file, O_RDONLY);
    if(file_fd == -1){
        perror("open file failed");
        return NULL;
    }

    // 计算文件有多少字节
    off_t file_size = lseek(file_fd, 0, SEEK_END);
    if(file_size == -1){
        perror("lseek failed");
        close(file_fd);
        return NULL;
    }
    // 空文件
    if(file_size == 0){
        printf("%s为空文件\n", file);
        close(file_fd);
        return result;
    }

    // 读取文件内容
    char *file_buf = malloc(file_size + 1);
    if(!file_buf){
        perror("malloc file_buf failed");
        close(file_fd);
        free(result);
        return NULL;
    }
    memset(file_buf, 0, file_size + 1); // 初始化缓冲区

    // 将偏移量回位
    lseek(file_fd, 0, SEEK_SET);
    ssize_t read_len = read(file_fd, file_buf, file_size);
    if(read_len == -1){
        perror("read file failed");
        close(file_fd);
        free(file_buf);
        return NULL;
    }
    else if(read_len != file_size){
        fprintf(stderr, "读取文件不完整，预期%zu字节，实际读取%zd字节。", file_size, read_len);
        close(file_fd);
        free(file_buf);
        free(result);
        return NULL;
    }
    close(file_fd); // 读取完成

    result->cmds = (char**)malloc(sizeof(char *) * KVSTORE_MAX_CMDS);
    if(result->cmds == NULL){
        perror("malloc cmds failed");
        free(file_buf);
        free(result);
        return NULL;
    }
    memset(result->cmds , 0, sizeof(char *) * KVSTORE_MAX_CMDS);

    // 按照 /r/n 分割指令填入数组
    char *start = file_buf;
    char *sep = NULL;
    
    while(((sep = strstr(start, "\r\n")) != NULL) && (result->cmd_count < KVSTORE_MAX_CMDS)){
        int cmd_len = sep - start;
        if(cmd_len > 0){
            result->cmds[result->cmd_count] = (char*)malloc(cmd_len + 1);
            if(result->cmds[result->cmd_count] == NULL){
                perror("malloc cmd[] failed");
                free(result);
                free(file_buf);
                return NULL;
            }
            strncpy(result->cmds[result->cmd_count], start, cmd_len);
            // result->cmds[result->cmd_count] += '\0';
            result->cmds[result->cmd_count][cmd_len] = '\0';
            result->cmd_count++;
        }
        start = sep + 2; // 跳过\r\n
    }

    // 处理最后一条无\r\n结尾的指令
    if (start < file_buf + file_size && result->cmd_count < KVSTORE_MAX_CMDS){
        int cmd_len = (file_buf + file_size) - start;
        if (cmd_len > 0){
            result->cmds[result->cmd_count] = (char *)malloc(cmd_len + 1);
            if (result->cmds[result->cmd_count] == NULL){
                perror("malloc last cmd failed");
                for (int i = 0; i < result->cmd_count; i++)
                {
                free(result->cmds[i]);
                }
                free(result->cmds);
                free(file_buf);
                free(result);
                return NULL;
        }
            strncpy(result->cmds[result->cmd_count], start, cmd_len);
            result->cmds[result->cmd_count][cmd_len] = '\0';
            result->cmd_count++;
        }
    }

    free(file_buf);
    return result;
}

void kvstore_free_recover_result(KvRecoverResult_t *recover_result){
    if(recover_result == NULL) return;
    if(recover_result->cmds == NULL) return;

    for(int i = 0; i < recover_result->cmd_count; i++){
        if(recover_result->cmds[i] != NULL){
            free(recover_result->cmds[i]);
        }
    }
    free(recover_result->cmds);
    free(recover_result);
}

int kvstore_execute_recovered_cmds(const char* file){
    KvRecoverResult_t *recover_result = kvstore_recover_data(file);
    if(recover_result == NULL){
        fprintf(stderr, "recovery command failed or command is empty");
        return -1;
    }
    printf("共有 %d 条指令\n", recover_result->cmd_count);

    // 遍历每条指令来执行
    for(int i = 0; i < recover_result->cmd_count; i++){
        char *cmd_cur = recover_result->cmds[i];
        if(cmd_cur == NULL){
            fprintf(stderr, "instruction %d is empty", i + 1);
            continue;
        }

        printf("原始指令为：%s\n", cmd_cur);

        char *tokens[KVSTORE_MAX_TOKENS] = {NULL};
        char response[KVSTORE_BUF_SIZE] = {0};

        // memset(tokens, 0, KVSTORE_MAX_TOKENS);
        // memset(tokens, 0, KVSTORE_BUF_SIZE);

        char cmd_copy[strlen(cmd_cur) + 1]; // 避免strtok改变原内容
        strcpy(cmd_copy, cmd_cur);

        int count = kvstore_split_token(cmd_copy, tokens);
        if(count < 0){
            fprintf(stderr, "the tokens or cmd for the %d instruction are empty", i + 1);
            continue;
        }

        int ret = kvstore_parser_protocol(tokens, count, response);
        if(ret > 0){
            printf("指令执行成功，返回值为：%s\n", response);
        }
    }

    // 释放内存
    /*
    free(recover_result); 
    错误做法，recover_result里有一个int和char **
    这样只释放传进去的指针所指向的那一块内存
    会漏掉里面的命令字符串和指针数组
    */
    kvstore_free_recover_result(recover_result);

    printf("所有命令都已释放\n");
    return 0;
}

// 在执行DEL命令时删除log文件中的SET命令
int delete_log_cmd(const char *log_file, const char *cmd_prefix, char *key){
    if(log_file == NULL || cmd_prefix == NULL || key == NULL){
        fprintf(stderr, "delete_log_cmd:参数为空\n");
        return -1;
    }

    struct stat *file_stat;
    if(stat(log_file, file_stat) == 0){
        // 检查当前用户是否有写权限
        if((file_stat->st_mode & S_IWUSR) == 0){
            fprintf(stderr, "文件%s无用户写权限，尝试自动修复\n", log_file);
            if (chmod(log_file, 0664) == -1){
                fprintf(stderr, "自动修复权限失败，请手动执行：chmod 664 %s\n", log_file);
                return -1;
            }
            fprintf(stderr, "文件 %s 权限已自动修复为 rw-rw-r--\n", log_file);
        }
    }

    // 读取日志所有指令
    KvRecoverResult_t *result = kvstore_recover_data(log_file);
    if(result == NULL) return 0; // 日志为空，没有要删除的数据

      char **filtered_cmds = (char **)malloc(sizeof(char *) * KVSTORE_MAX_CMDS);
    if (filtered_cmds == NULL)
    {
        perror("malloc filtered_cmds failed");
        kvstore_free_recover_result(result);
        return -1;
    }
    memset(filtered_cmds, 0, sizeof(char *) * KVSTORE_MAX_CMDS);

    int filtered_count = 0;   // 过滤后的指令数
    const int MAX_TOKENS = 3; // set指令格式：set key val → 3个token
    char *tokens[MAX_TOKENS];
    memset(tokens, 0, sizeof(tokens));

    for (int i = 0; i < result->cmd_count; i++)
    {
        char *curr_cmd = result->cmds[i];
        if (curr_cmd == NULL)
        continue;

        // 复制指令（避免strtok修改原内容）
        char cmd_copy[strlen(curr_cmd) + 1];
        strcpy(cmd_copy, curr_cmd);

        // 分割token
        int token_count = kvstore_split_token(cmd_copy, tokens);

        // 过滤逻辑：
        // 1. 不是目标set指令 → 保留
        // 2. 是目标set指令但key不匹配 → 保留
        // 3. 是目标set指令且key匹配 → 跳过（删除）
        int is_target_cmd = 0;
        if (token_count == 3 && strcmp(tokens[0], cmd_prefix) == 0 && strcmp(tokens[1], key) == 0)
        {
        is_target_cmd = 1; // 匹配目标指令，需要删除
        }

        if (!is_target_cmd)
        {
        // 保留该指令：分配内存并复制
        filtered_cmds[filtered_count] = (char *)malloc(strlen(curr_cmd) + 1);
        if (filtered_cmds[filtered_count] == NULL)
        {
            perror("malloc filtered cmd failed");
            // 清理已分配的过滤指令
            for (int j = 0; j < filtered_count; j++)
            {
            free(filtered_cmds[j]);
            }
            free(filtered_cmds);
            kvstore_free_recover_result(result);
            return -1;
        }
        strcpy(filtered_cmds[filtered_count], curr_cmd);
        filtered_count++;
        }
    }

    // 步骤3：覆盖写入日志文件（清空原文件，写入过滤后的指令）
    int fd = open(log_file, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (fd == -1)
    {
        perror("open log file for write failed");
        // 清理过滤指令内存
        for (int j = 0; j < filtered_count; j++)
        {
        free(filtered_cmds[j]);
        }
        free(filtered_cmds);
        kvstore_free_recover_result(result);
        return -1;
    }

    // 写入过滤后的指令（每条加\r\n）
    for (int i = 0; i < filtered_count; i++)
    {
        if (filtered_cmds[i] == NULL)
        continue;
        // 拼接指令 + \r\n
        char cmd_line[1024] = {0};
        snprintf(cmd_line, sizeof(cmd_line), "%s\r\n", filtered_cmds[i]);
        write(fd, cmd_line, strlen(cmd_line));
    }
    close(fd);

    // 步骤4：清理所有内存
    for (int j = 0; j < filtered_count; j++)
    {
        free(filtered_cmds[j]);
    }
    free(filtered_cmds);
    kvstore_free_recover_result(result);

    printf("成功删除日志中 [%s %s] 相关的set指令\n", cmd_prefix, key);
    return 0;
}

// 执行MOD时修改日志中的SET
int modify_log_cmd(const char *log_file, const char *cmd_prefix, char *key, char *value){
    if(log_file == NULL || cmd_prefix == NULL || key == NULL || value == NULL){
        fprintf(stderr, "modify_log_cmd 参数为空\n");
        return -1;
    }

    // 读取日志的所有信息
    KvRecoverResult_t *result = kvstore_recover_data(log_file);
    if(result == NULL){
        fprintf(stderr, "日志为空或读取日志失败\n");
        return -1;
    }

    char **cmd_mod = (char **)malloc(sizeof(char*) * KVSTORE_MAX_CMDS);
    if(cmd_mod == NULL){
        fprintf(stderr, "malloc cmd_mod failed\n");
        return -1;
    }
    memset(cmd_mod, 0, sizeof(char*) * KVSTORE_MAX_CMDS);

    int modified_count = 0;   // 修改后指令总数
    int is_found = 0;         // 是否找到并修改了目标指令
    const int MAX_TOKENS = 3; // set指令格式：set key val → 3个token
    char *tokens[MAX_TOKENS];
    memset(tokens,0,sizeof(tokens));

    // 4. 遍历指令,找到目标指令并修改value
    for (int i = 0; i < result->cmd_count; i++)
    {
        char *curr_cmd = result->cmds[i];
        if (curr_cmd == NULL)
        continue;

        // 复制指令（避免strtok修改原内容)
        char cmd_copy[strlen(curr_cmd) + 1];
        strcpy(cmd_copy, curr_cmd);

        // 分割指令为token（复用已有函数)
        int token_count = kvstore_split_token(cmd_copy, tokens);

        // 5. 判断是否是目标指令：set前缀 + 3个token + key匹配
        if (token_count == 3 && strcmp(tokens[0], cmd_prefix) == 0 && strcmp(tokens[1], key) == 0)
        {
        // 找到目标指令 → 构造新指令（cmd_prefix key new_value)
        char new_cmd[1024] = {0};
        snprintf(new_cmd, sizeof(new_cmd), "%s %s %s", cmd_prefix, key, value);

        // 为新指令分配内存并复制
        cmd_mod[modified_count] = (char *)malloc(strlen(new_cmd) + 1);
        if (cmd_mod[modified_count] == NULL)
        {
            perror("mod_log_set_cmd: malloc new_cmd失败");
            // 清理已分配的内存
            for (int j = 0; j < modified_count; j++)
            {
            free(cmd_mod[j]);
            }
            free(cmd_mod);
            kvstore_free_recover_result(result);
            return -1;
        }
        strcpy(cmd_mod[modified_count], new_cmd);
        modified_count++;
        is_found = 1; // 标记：找到并修改了目标指令
        printf("mod_log_set_cmd: 找到目标指令,已修改为：%s\n", new_cmd);
        }
        else
        {
        // 非目标指令 → 直接保留原指令
        cmd_mod[modified_count] = (char *)malloc(strlen(curr_cmd) + 1);
        if (cmd_mod[modified_count] == NULL)
        {
            perror("mod_log_set_cmd: malloc curr_cmd失败");
            // 清理已分配的内存
            for (int j = 0; j < modified_count; j++)
            {
            free(cmd_mod[j]);
            }
            free(cmd_mod);
            kvstore_free_recover_result(result);
            // 清理tokens内存
            for (int j = 0; j < token_count; j++)
            {
            free(tokens[j]);
            }
            return -1;
        }
        strcpy(cmd_mod[modified_count], curr_cmd);
        modified_count++;
        }
    }

    // 6. 未找到目标指令 → 清理内存并返回1
    if (!is_found)
    {
        fprintf(stderr, "mod_log_set_cmd: 未找到 [%s %s] 对应的set指令\n", cmd_prefix, key);
        for (int j = 0; j < modified_count; j++)
        {
        free(cmd_mod[j]);
        }
        free(cmd_mod);
        kvstore_free_recover_result(result);
        return 1;
    }

    // 7. 覆盖写入修改后的指令到日志文件（保证权限：0664)
    int fd = open(log_file, O_WRONLY | O_TRUNC | O_CREAT, 0664);
    if (fd == -1)
    {
        perror("mod_log_set_cmd: 打开日志文件写入失败");
        fprintf(stderr, "修复建议：执行 chmod 664 %s\n", log_file);
        // 清理内存
        for (int j = 0; j < modified_count; j++)
        {
        free(cmd_mod[j]);
        }
        free(cmd_mod);
        kvstore_free_recover_result(result);
        return -1;
    }

    // 8. 写入所有修改后的指令（每条加\r\n,和原有日志格式一致)
    for (int i = 0; i < modified_count; i++)
    {
        if (cmd_mod[i] == NULL)
        continue;
        char cmd_line[1024] = {0};
        snprintf(cmd_line, sizeof(cmd_line), "%s\r\n", cmd_mod[i]);
        write(fd, cmd_line, strlen(cmd_line));
    }
    close(fd);

    // 9. 清理所有内存
    for (int j = 0; j < modified_count; j++)
    {
        free(cmd_mod[j]);
    }
    free(cmd_mod);
    kvstore_free_recover_result(result);

    printf("mod_log_set_cmd: 成功修改日志中 [%s %s] 的value为 %s\n", cmd_prefix, key, value);
    return 0;
}
#endif