<template>
  <div class="page-container">
    <!-- Vehicle Table -->
    <div class="glass-card">
      <div class="glass-card-header">
        <h3>车辆管理</h3>
        <div class="header-controls">
          <el-input
            v-model="query.keyword"
            placeholder="搜索车辆..."
            :prefix-icon="Search"
            clearable
            style="width: 200px"
            @keyup.enter="loadVehicles"
            @clear="loadVehicles"
          />
          <el-select v-model="query.status" placeholder="全部状态" clearable style="width: 120px" @change="loadVehicles">
            <el-option label="可用" value="available" />
            <el-option label="已租出" value="rented" />
            <el-option label="维修中" value="maintenance" />
            <el-option label="已退役" value="retired" />
          </el-select>
          <el-input
            v-model="query.brand"
            placeholder="品牌"
            clearable
            style="width: 120px"
            @keyup.enter="loadVehicles"
            @clear="loadVehicles"
          />
          <el-button type="primary" @click="loadVehicles">
            <el-icon><Search /></el-icon> 搜索
          </el-button>
          <el-button v-if="authStore.isAdmin" type="primary" @click="$router.push('/vehicles/add')">
            <el-icon><Plus /></el-icon> 添加车辆
          </el-button>
        </div>
      </div>
      <div class="glass-card-body">
        <el-table :data="vehicles" v-loading="loading" style="width: 100%" empty-text="暂无车辆">
          <el-table-column prop="plate_number" label="车牌号" min-width="120" />
          <el-table-column prop="brand" label="品牌" min-width="100" />
          <el-table-column prop="model" label="型号" min-width="100" />
          <el-table-column prop="color" label="颜色" min-width="80" />
          <el-table-column prop="status" label="状态" min-width="110">
            <template #default="{ row }">
              <el-tag :type="vehicleStatusType(row.status)" effect="dark" size="small">{{ row.status }}</el-tag>
            </template>
          </el-table-column>
          <el-table-column prop="daily_rate" label="日租金" min-width="100">
            <template #default="{ row }">
              <span class="text-accent">¥{{ row.daily_rate }}</span>
            </template>
          </el-table-column>
          <el-table-column label="操作" min-width="160" v-if="authStore.isAdmin">
            <template #default="{ row }">
              <el-button link type="primary" @click="$router.push(`/vehicles/${row.id}`)">查看</el-button>
              <el-button link type="primary" @click="$router.push(`/vehicles/${row.id}/edit`)">编辑</el-button>
              <el-popconfirm title="确认删除此车辆？" @confirm="handleDelete(row.id)">
                <template #reference>
                  <el-button link type="danger">删除</el-button>
                </template>
              </el-popconfirm>
            </template>
          </el-table-column>
          <el-table-column label="操作" min-width="80" v-else>
            <template #default="{ row }">
              <el-button link type="primary" @click="$router.push(`/vehicles/${row.id}`)">查看</el-button>
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
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { useAuthStore } from '@/stores/auth'
import { getVehicleList, deleteVehicle } from '@/api/vehicle'
import { ElMessage } from 'element-plus'
import { Search, Plus } from '@element-plus/icons-vue'

const authStore = useAuthStore()
const loading = ref(false)
const vehicles = ref([])
const total = ref(0)

const query = reactive({
  page: 1,
  page_size: 20,
  keyword: '',
  status: '',
  brand: '',
})

function vehicleStatusType(status) {
  const map = {
    'available': 'success',
    'rented': 'warning',
    'maintenance': 'info',
    'retired': 'danger',
  }
  return map[(status || '').toLowerCase()] || 'info'
}

async function loadVehicles() {
  loading.value = true
  try {
    const params = { ...query }
    if (!params.keyword) delete params.keyword
    if (!params.status) delete params.status
    if (!params.brand) delete params.brand
    const res = await getVehicleList(params)
    if (res.error === 0) {
      vehicles.value = res.list || res.vehicles || []
      total.value = res.total || 0
    }
  } catch {
    ElMessage.error('加载车辆失败')
  } finally {
    loading.value = false
  }
}

async function handleDelete(id) {
  try {
    const res = await deleteVehicle({ id })
    if (res.error === 0) {
      ElMessage.success('车辆已删除')
      loadVehicles()
    } else {
      ElMessage.error('删除车辆失败')
    }
  } catch {
    ElMessage.error('删除车辆失败')
  }
}

function handlePageChange(page) {
  query.page = page
  loadVehicles()
}

onMounted(loadVehicles)
</script>

<style scoped>
.header-controls {
  display: flex;
  gap: 10px;
  align-items: center;
  flex-wrap: wrap;
}

.pagination-wrapper {
  display: flex;
  justify-content: center;
  padding: 20px 0 8px;
}
</style>
