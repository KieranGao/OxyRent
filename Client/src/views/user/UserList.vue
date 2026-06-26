<template>
  <div class="page-container">
    <div class="page-header">
      <h2>用户管理</h2>
      <p>管理系统用户</p>
    </div>

    <div class="search-bar">
      <el-input
        v-model="query.keyword"
        placeholder="搜索用户..."
        :prefix-icon="Search"
        clearable
        style="width: 200px"
        @keyup.enter="loadUsers"
        @clear="loadUsers"
      />
      <el-select v-model="query.role" placeholder="全部角色" clearable style="width: 120px" @change="loadUsers">
        <el-option label="客户" value="customer" />
        <el-option label="员工" value="staff" />
        <el-option label="管理员" value="admin" />
      </el-select>
      <el-select v-model="query.status" placeholder="全部状态" clearable style="width: 120px" @change="loadUsers">
        <el-option label="正常" value="active" />
        <el-option label="已禁用" value="disabled" />
      </el-select>
      <el-button type="primary" @click="loadUsers">
        <el-icon><Search /></el-icon> 搜索
      </el-button>
    </div>

    <el-card>
      <el-table :data="users" v-loading="loading" style="width: 100%" empty-text="暂无用户">
        <el-table-column prop="uid" label="UID" min-width="60" />
        <el-table-column prop="username" label="用户名" min-width="120" />
        <el-table-column prop="real_name" label="真实姓名" min-width="100" />
        <el-table-column prop="phone" label="手机号" min-width="120" />
        <el-table-column prop="email" label="邮箱" min-width="150" />
        <el-table-column prop="role" label="角色" min-width="80">
          <template #default="{ row }">
            <el-tag :type="row.role === 'admin' ? 'danger' : row.role === 'staff' ? 'warning' : 'info'">
              {{ row.role === 'admin' ? '管理员' : row.role === 'staff' ? '员工' : '客户' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="status" label="状态" min-width="90">
          <template #default="{ row }">
            <el-tag :type="row.status === 'active' ? 'success' : 'danger'">{{ row.status }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作" min-width="180">
          <template #default="{ row }">
            <el-button link type="primary" @click="showRoleDialog(row)">设置角色</el-button>
            <el-button
              v-if="row.status === 'active'"
              link type="danger"
              @click="handleStatusChange(row.uid, 'disabled')"
            >
              禁用
            </el-button>
            <el-button
              v-if="row.status === 'disabled'"
              link type="success"
              @click="handleStatusChange(row.uid, 'active')"
            >
              启用
            </el-button>
          </template>
        </el-table-column>
      </el-table>

      <div class="pagination-wrapper" v-if="total > query.page_size">
        <el-pagination
          background
          layout="prev, pager, next"
          :total="total"
          :page-size="query.page_size"
          :current-page="query.page"
          @current-change="handlePageChange"
        />
      </div>
    </el-card>

    <!-- Role Dialog -->
    <el-dialog v-model="roleDialogVisible" title="修改用户角色" width="400px">
      <el-form label-position="top">
        <el-form-item label="角色">
          <el-select v-model="newRole" placeholder="请选择角色" style="width: 100%">
            <el-option label="客户" value="customer" />
            <el-option label="员工" value="staff" />
            <el-option label="管理员" value="admin" />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="roleDialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="updatingRole" @click="handleRoleChange">确认</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { getUserList, updateUserStatus, updateUserRole } from '@/api/user'
import { ElMessage, ElMessageBox } from 'element-plus'
import { Search } from '@element-plus/icons-vue'

const loading = ref(false)
const users = ref([])
const total = ref(0)
const roleDialogVisible = ref(false)
const updatingRole = ref(false)
const editingUid = ref(null)
const newRole = ref('customer')

const query = reactive({
  page: 1,
  page_size: 20,
  keyword: '',
  role: '',
  status: '',
})

async function loadUsers() {
  loading.value = true
  try {
    const params = { ...query }
    if (!params.keyword) delete params.keyword
    if (params.role === '') delete params.role
    if (!params.status) delete params.status
    const res = await getUserList(params)
    if (res.error === 0) {
      users.value = res.list || res.users || []
      total.value = res.total || 0
    }
  } catch {
    ElMessage.error('加载用户列表失败')
  } finally {
    loading.value = false
  }
}

function showRoleDialog(row) {
  editingUid.value = row.uid
  newRole.value = row.role
  roleDialogVisible.value = true
}

async function handleRoleChange() {
  updatingRole.value = true
  try {
    const res = await updateUserRole({ uid: editingUid.value, role: newRole.value })
    if (res.error === 0) {
      ElMessage.success('角色已更新')
      roleDialogVisible.value = false
      loadUsers()
    } else {
      ElMessage.error('更新角色失败')
    }
  } catch {
    ElMessage.error('更新角色失败')
  } finally {
    updatingRole.value = false
  }
}

async function handleStatusChange(uid, status) {
  try {
    await ElMessageBox.confirm(
      `确认${status === 'active' ? '启用' : '禁用'}此用户？`,
      '确认',
      { type: 'warning' },
    )
    const res = await updateUserStatus({ uid, status })
    if (res.error === 0) {
      ElMessage.success('状态已更新')
      loadUsers()
    } else {
      ElMessage.error('更新状态失败')
    }
  } catch {
    // Cancelled
  }
}

function handlePageChange(page) {
  query.page = page
  loadUsers()
}

onMounted(loadUsers)
</script>

<style scoped>
.search-bar {
  display: flex;
  gap: 12px;
  align-items: center;
  margin-bottom: 20px;
  flex-wrap: wrap;
}

.pagination-wrapper {
  display: flex;
  justify-content: center;
  margin-top: 20px;
}
</style>
