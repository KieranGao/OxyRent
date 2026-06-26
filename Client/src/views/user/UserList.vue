<template>
  <div class="page-container">
    <div class="page-header">
      <h2>Users</h2>
      <p>Manage system users</p>
    </div>

    <div class="search-bar">
      <el-input
        v-model="query.keyword"
        placeholder="Search users..."
        :prefix-icon="Search"
        clearable
        style="width: 200px"
        @keyup.enter="loadUsers"
        @clear="loadUsers"
      />
      <el-select v-model="query.role" placeholder="All Roles" clearable style="width: 120px" @change="loadUsers">
        <el-option label="Customer" value="customer" />
        <el-option label="Staff" value="staff" />
        <el-option label="Admin" value="admin" />
      </el-select>
      <el-select v-model="query.status" placeholder="All Status" clearable style="width: 120px" @change="loadUsers">
        <el-option label="Active" value="active" />
        <el-option label="Disabled" value="disabled" />
      </el-select>
      <el-button type="primary" @click="loadUsers">
        <el-icon><Search /></el-icon> Search
      </el-button>
    </div>

    <el-card>
      <el-table :data="users" v-loading="loading" style="width: 100%" empty-text="No users found">
        <el-table-column prop="uid" label="UID" min-width="60" />
        <el-table-column prop="username" label="Username" min-width="120" />
        <el-table-column prop="real_name" label="Real Name" min-width="100" />
        <el-table-column prop="phone" label="Phone" min-width="120" />
        <el-table-column prop="email" label="Email" min-width="150" />
        <el-table-column prop="role" label="Role" min-width="80">
          <template #default="{ row }">
            <el-tag :type="row.role === 'admin' ? 'danger' : row.role === 'staff' ? 'warning' : 'info'">
              {{ row.role === 'admin' ? 'Admin' : row.role === 'staff' ? 'Staff' : 'Customer' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="status" label="Status" min-width="90">
          <template #default="{ row }">
            <el-tag :type="row.status === 'active' ? 'success' : 'danger'">{{ row.status }}</el-tag>
          </template>
        </el-table-column>
        <el-table-column label="Actions" min-width="180">
          <template #default="{ row }">
            <el-button link type="primary" @click="showRoleDialog(row)">Set Role</el-button>
            <el-button
              v-if="row.status === 'active'"
              link type="danger"
              @click="handleStatusChange(row.uid, 'disabled')"
            >
              Disable
            </el-button>
            <el-button
              v-if="row.status === 'disabled'"
              link type="success"
              @click="handleStatusChange(row.uid, 'active')"
            >
              Enable
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
    <el-dialog v-model="roleDialogVisible" title="Change User Role" width="400px">
      <el-form label-position="top">
        <el-form-item label="Role">
          <el-select v-model="newRole" placeholder="Select role" style="width: 100%">
            <el-option label="Customer" value="customer" />
            <el-option label="Staff" value="staff" />
            <el-option label="Admin" value="admin" />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="roleDialogVisible = false">Cancel</el-button>
        <el-button type="primary" :loading="updatingRole" @click="handleRoleChange">Confirm</el-button>
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
    ElMessage.error('Failed to load users')
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
      ElMessage.success('Role updated')
      roleDialogVisible.value = false
      loadUsers()
    } else {
      ElMessage.error('Failed to update role')
    }
  } catch {
    ElMessage.error('Failed to update role')
  } finally {
    updatingRole.value = false
  }
}

async function handleStatusChange(uid, status) {
  try {
    await ElMessageBox.confirm(
      `Are you sure you want to ${status === 'active' ? 'enable' : 'disable'} this user?`,
      'Confirm',
      { type: 'warning' },
    )
    const res = await updateUserStatus({ uid, status })
    if (res.error === 0) {
      ElMessage.success('Status updated')
      loadUsers()
    } else {
      ElMessage.error('Failed to update status')
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
